#define Py_LIMITED_API 0x030c0000
#define PY_SSIZE_T_CLEAN

#include "../includes/parse.h"

#include "../includes/c_optimizations.h"

#include <Python.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define ftruncate _chsize
#define fileno _fileno
#define SEARCH_QUERY_EXTRA_CHARS 3
#else
#include <dirent.h>
#include <string.h>
#define SEARCH_QUERY_EXTRA_CHARS 2
#define PTCL_UTF8
#endif

#ifdef PTCL_UTF8
#define CHAR_LITERAL(x) x
#define ptcl_char char
#define ptcl_memcpy memcpy
#define ptcl_memcmp memcmp
#define ptcl_strlen strlen
#define ptcl_open_r_plus(path) fopen(path, "r+")
#define ptcl_FIND_DATA _WIN32_FIND_DATAA
#define ptcl_FindFirstFileEx FindFirstFileExA
#define ptcl_FindNextFile FindNextFileA
#define PyUnicode_AsPtclChar PyUnicode_AsUTF8AndSize
#define PyUnicode_PtclChar const char *
#else
#define CHAR_LITERAL(x) L##x
#define ptcl_char wchar_t
#define ptcl_memcpy wmemcpy
#define ptcl_memcmp wmemcmp
#define ptcl_strlen wcslen
#define ptcl_open_r_plus(path) _wfopen(path, L"r+")
#define ptcl_FIND_DATA _WIN32_FIND_DATAW
#define ptcl_FindFirstFileEx FindFirstFileExW
#define ptcl_FindNextFile FindNextFileW
#define PyUnicode_AsPtclChar PyUnicode_AsWideCharString
#define PyUnicode_PtclChar wchar_t *
#endif

static PyObject *Py_tcl_minify(PyObject *self, PyObject *arg) {
    Py_ssize_t size;
    const char *source = PyUnicode_AsUTF8AndSize(arg, &size);
    if (unlikely(source == NULL)) {
        return NULL;
    }

    size_t minified_size;
    char *minified_source = tcl_minify(source, size, &minified_size);

    PyObject *out = PyUnicode_FromStringAndSize(minified_source, minified_size);

    free(minified_source);

    return out;
}

static int _tcl_minify_file(const ptcl_char *path) {
    FILE *fp = ptcl_open_r_plus(path);

    if (fp == NULL) {
        PyErr_SetString(PyExc_OSError, "Error opening TCL file to read");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    const long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (unlikely(file_size < 0)) {
        PyErr_SetString(PyExc_OSError, "Error telling TCL file");
        fclose(fp);
        return 1;
    }

    char *source = (char *)malloc(file_size * sizeof(char));
    if (unlikely(source == NULL)) {
        fclose(fp);
        return 1;
    }

    const size_t read_bytes = fread(source, sizeof(char), file_size, fp);
    if (unlikely(ferror(fp))) {
        fclose(fp);
        PyErr_SetString(PyExc_OSError, "Error reading TCL file");
        return 1;
    }

    size_t minified_size;
    char *minified_source = tcl_minify(source, read_bytes, &minified_size);
    free(source);

    if (ftruncate(fileno(fp), 0) < 0) {
        fclose(fp);
        PyErr_SetString(PyExc_OSError, "Error truncating TCL file");
        return 1;
    }
    rewind(fp);

    const size_t written_bytes = fwrite(minified_source, sizeof(char), minified_size, fp);
    fclose(fp);
    free(minified_source);

    if (written_bytes != minified_size) {
        PyErr_SetString(PyExc_OSError, "Error writing TCL file");
        return 1;
    }

    return 0;
}

static inline void _PyMem_Free_IfNeeded(PyUnicode_PtclChar path) {
#ifndef PTCL_UTF8
    PyMem_Free(path);
#endif
}

static PyObject *Py_tcl_minify_file(PyObject *self, PyObject *arg) {
    PyUnicode_PtclChar path = PyUnicode_AsPtclChar(arg, NULL);
    if (unlikely(path == NULL)) {
        return NULL;
    }

    const int error = _tcl_minify_file(path);

    _PyMem_Free_IfNeeded(path);

    return error ? NULL : Py_None;
}

struct ReverseLinkedList
{
    struct ReverseLinkedList *previous;
    ptcl_char *search_query;
};

static struct ReverseLinkedList *ReverseLinkedList_append(
    struct ReverseLinkedList *previous,
    const ptcl_char *path,
    size_t path_size
) {
    struct ReverseLinkedList *appended = malloc(sizeof(struct ReverseLinkedList));
    appended->previous = previous;
    appended->search_query = malloc((path_size + SEARCH_QUERY_EXTRA_CHARS) * sizeof(ptcl_char));

    ptcl_memcpy(appended->search_query, path, path_size);

    const ptcl_char path_last_char = path[path_size - 1];
#ifdef _WIN32
    if (path_last_char != CHAR_LITERAL('/') && path_last_char != CHAR_LITERAL('\\')) {
        ptcl_memcpy(appended->search_query + path_size, CHAR_LITERAL("\\*"), ptcl_strlen(CHAR_LITERAL("\\*")) + 1);
    } else {
        ptcl_memcpy(appended->search_query + path_size, CHAR_LITERAL("*"), ptcl_strlen(CHAR_LITERAL("*")) + 1);
    }
#else
    if (path_last_char != CHAR_LITERAL('/')) {
        appended->search_query[path_size++] = '/';
    }
    appended->search_query[path_size] = '\0';
#endif

    return appended;
}

static inline struct ReverseLinkedList *ReverseLinkedList_pop(struct ReverseLinkedList *list) {
    struct ReverseLinkedList *previous = list->previous;

    free(list);

    return previous;
}

static inline bool _ignore_path(const ptcl_char *path, size_t path_size) {
    switch (path_size) {
    case 1:
        return ptcl_memcmp(path, CHAR_LITERAL("."), 1) == 0;
    case 2:
        return ptcl_memcmp(path, CHAR_LITERAL(".."), 2) == 0;
    default:
        return false;
    }
}

static inline bool _has_tcl_file_ext(const ptcl_char *file_name, size_t file_name_size) {
    return ptcl_memcmp(file_name + file_name_size - 3, CHAR_LITERAL(".tm"), 3) == 0 ||
           ptcl_memcmp(file_name + file_name_size - 4, CHAR_LITERAL(".tcl"), 4) == 0;
}

static inline int _tcl_minify_folder(const ptcl_char *search_path, size_t search_path_size) {

    struct ReverseLinkedList *folders_to_visit_stack = ReverseLinkedList_append(NULL, search_path, search_path_size);

    while (folders_to_visit_stack != NULL) {
        ptcl_char *search_query = folders_to_visit_stack->search_query;
        const size_t search_query_size = ptcl_strlen(search_query);

        folders_to_visit_stack = ReverseLinkedList_pop(folders_to_visit_stack);

#ifdef _WIN32
        struct ptcl_FIND_DATA file_data;
        HANDLE file_handle = ptcl_FindFirstFileEx(
            search_query,
            FindExInfoBasic,
            &file_data,
            FindExSearchNameMatch,
            NULL,
            FIND_FIRST_EX_LARGE_FETCH
        );

        if (file_handle == INVALID_HANDLE_VALUE) {
            PyErr_SetString(PyExc_OSError, "Can't find or access folder");
            return 1;
        }

        do {
            const size_t file_name_size = ptcl_strlen(file_data.cFileName);
            if (_ignore_path(file_data.cFileName, file_name_size)) {
                continue;
            }

            const size_t path_size = search_query_size + file_name_size - 1;
            ptcl_char path[path_size + 1];

            ptcl_memcpy(path, search_query, search_query_size);
            ptcl_memcpy(path + search_query_size - 1, file_data.cFileName, file_name_size + 1);

            if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                if (_has_tcl_file_ext(file_data.cFileName, file_name_size)) {
                    if (_tcl_minify_file(path)) {
                        return 2;
                    }
                }
            } else {
                folders_to_visit_stack = ReverseLinkedList_append(folders_to_visit_stack, path, path_size);
            }
        } while (ptcl_FindNextFile(file_handle, &file_data));

        FindClose(file_handle);
#else
        struct dirent *dp;
        DIR *directory = opendir(search_query);

        if (!directory) {
            PyErr_SetString(PyExc_OSError, "Can't find or access folder");
            return 1;
        }

        while ((dp = readdir(directory)) != NULL) {
            const size_t file_name_size = ptcl_strlen(dp->d_name);
            if (_ignore_path(dp->d_name, file_name_size)) {
                continue;
            }

            const size_t path_size = search_query_size + file_name_size;
            ptcl_char path[path_size + 1];

            ptcl_memcpy(path, search_query, search_query_size);
            ptcl_memcpy(path + search_query_size, dp->d_name, file_name_size + 1);

            // If the entry is a directory, recurse into it
            if (dp->d_type == DT_DIR) {
                folders_to_visit_stack = ReverseLinkedList_append(folders_to_visit_stack, path, path_size);
            } else {
                if (_has_tcl_file_ext(dp->d_name, file_name_size)) {
                    if (_tcl_minify_file(path)) {
                        return 2;
                    }
                }
            }
        }

        closedir(directory);

#endif
        free(search_query);
    }

    return 0;
}

static PyObject *Py_tcl_minify_folder(PyObject *self, PyObject *arg) {
    Py_ssize_t path_size;
    PyUnicode_PtclChar path = PyUnicode_AsPtclChar(arg, &path_size);
    if (unlikely(path == NULL)) {
        return NULL;
    }

    const int error = _tcl_minify_folder(path, path_size);

    _PyMem_Free_IfNeeded(path);

    return error ? NULL : Py_None;
}

static PyMethodDef parse_methods[] = {
    {"tcl_minify", Py_tcl_minify, METH_O, NULL},
    {"tcl_minify_file", Py_tcl_minify_file, METH_O, NULL},
    {"tcl_minify_folder", Py_tcl_minify_folder, METH_O, NULL},
    {NULL, NULL, 0, NULL},
};

static int parse_exec(PyObject *Py_UNUSED(module)) {
    return 0;
}

static PyModuleDef_Slot parse_slots[] = {
    {Py_mod_exec, parse_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_GIL_DISABLED
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL},
};

static struct PyModuleDef parse_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "parse",
    .m_size = 0,
    .m_methods = parse_methods,
    .m_slots = parse_slots,
};

PyMODINIT_FUNC PyInit_parse(void) {
    return PyModuleDef_Init(&parse_module);
}
