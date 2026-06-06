#define Py_LIMITED_API 0x030c0000
#define PY_SSIZE_T_CLEAN

#include "../includes/parse.h"

#include "../includes/c_optimizations.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#define ftruncate _chsize
#define fileno _fileno
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

static int __tcl_minify_file(FILE *fp) {
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

#ifdef _WIN32
static inline int _tcl_minify_file(const wchar_t *path) {
    FILE *fp = _wfopen(path, L"r+");
    return __tcl_minify_file(fp);
}
#else
static inline int _tcl_minify_file(const char *path) {
    FILE *fp = fopen(path, "r+");
    return __tcl_minify_file(fp);
}
#endif

static PyObject *Py_tcl_minify_file(PyObject *self, PyObject *arg) {
#ifdef _WIN32
    wchar_t *path = PyUnicode_AsWideCharString(arg, NULL);
#else
    const char *path = PyUnicode_AsUTF8AndSize(arg, NULL);
#endif
    if (unlikely(path == NULL)) {
        return NULL;
    }

    const int error = _tcl_minify_file(path);

    return error ? NULL : Py_None;
}

static inline int _tcl_minify_folder(const wchar_t *search_path, size_t search_path_size) {
    wchar_t search_query[(search_path_size + 3) * sizeof(wchar_t)];
    wmemcpy(search_query, search_path, search_path_size);

    const wchar_t path_last_char = search_path[search_path_size - 1];
    if (path_last_char != L'/' && path_last_char != L'\\') {
        wmemcpy(search_query + search_path_size, L"\\*", wcslen(L"\\*") + 1);
    } else {
        wmemcpy(search_query + search_path_size, L"*", wcslen(L"*") + 1);
    }

    struct _WIN32_FIND_DATAW file_data;
    HANDLE file_handle = FindFirstFileExW(search_query, FindExInfoBasic, &file_data, FindExSearchNameMatch, NULL, FIND_FIRST_EX_LARGE_FETCH);

    if (file_handle == INVALID_HANDLE_VALUE) {
        return 1;
    }

    const size_t search_query_size = wcslen(search_query);

    do {
        if (wcscmp(file_data.cFileName, L".") == 0 || wcscmp(file_data.cFileName, L"..") == 0) {
            continue;
        }

        const size_t file_name_size = wcslen(file_data.cFileName);
        const size_t path_size = search_query_size + file_name_size;
        wchar_t *path = malloc((path_size + 1) * sizeof(wchar_t));

        wmemcpy(path, search_query, search_query_size);
        wmemcpy(path + search_query_size - 1, file_data.cFileName, file_name_size + 1);

        if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            if ((file_name_size > 2 && wmemcmp(file_data.cFileName + file_name_size - 3, L".tm", 3) == 0) ||
                (file_name_size > 3 && wmemcmp(file_data.cFileName + file_name_size - 4, L".tcl", 4) == 0)) {
                if (_w_tcl_minify_file(path)) {
                    return 2;
                }
            }
        } else {
            _tcl_minify_folder(path, path_size - 1);
        }
        free(path);
    } while (FindNextFileW(file_handle, &file_data));

    FindClose(file_handle);

    return 0;
}

static PyObject *Py_tcl_minify_folder(PyObject *self, PyObject *arg) {
    Py_ssize_t path_size;
    wchar_t *path = PyUnicode_AsWideCharString(arg, &path_size);
    if (unlikely(path == NULL)) {
        return NULL;
    }

    const int error = _tcl_minify_folder(path, path_size);
    PyMem_Free(path);

    if (error) {
        PyErr_SetString(PyExc_OSError, "Error accessing folder");
    }

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
