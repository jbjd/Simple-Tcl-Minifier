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
#else
#define PTCL_UTF8
#endif

#ifdef PTCL_UTF8
#define ptcl_char char
#define ptcl_open_r_plus(path) fopen(path, "r+")
#define PyUnicode_AsPtclChar PyUnicode_AsUTF8AndSize
#define PyUnicode_PtclChar const char *
#else
#define ptcl_char wchar_t
#define ptcl_open_r_plus(path) _wfopen(path, L"r+")
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

static PyMethodDef parse_methods[] = {
    {"tcl_minify", Py_tcl_minify, METH_O, NULL},
    {"tcl_minify_file", Py_tcl_minify_file, METH_O, NULL},
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
