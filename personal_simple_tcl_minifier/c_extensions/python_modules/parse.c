#define Py_LIMITED_API 0x030c0000
#define PY_SSIZE_T_CLEAN

#include "../includes/parse.h"

#include "../includes/c_optimizations.h"

#include <Python.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
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

static PyObject *Py_tcl_minify_file(PyObject *self, PyObject *arg) {
    const char *path = PyUnicode_AsUTF8AndSize(arg, NULL);
    if (unlikely(path == NULL)) {
        return NULL;
    }

    FILE *fp = fopen(path, "r+");
    if (fp == NULL) {
        PyErr_SetString(PyExc_OSError, "Error opening TCL file to read");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    const long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (unlikely(file_size < 0)) {
        PyErr_SetString(PyExc_OSError, "Error telling TCL file");
        fclose(fp);
        return NULL;
    }

    char *source = (char *)malloc(file_size * sizeof(char));
    if (unlikely(source == NULL)) {
        fclose(fp);
        return NULL;
    }

    const size_t read_bytes = fread(source, sizeof(char), file_size, fp);
    if (unlikely(ferror(fp))) {
        fclose(fp);
        PyErr_SetString(PyExc_OSError, "Error reading TCL file");
        return NULL;
    }

    size_t minified_size;
    char *minified_source = tcl_minify(source, read_bytes, &minified_size);

    if (ftruncate(fileno(fp), 0) < 0) {
        fclose(fp);
        PyErr_SetString(PyExc_OSError, "Error truncating TCL file");
        return NULL;
    }
    rewind(fp);

    const size_t written_bytes = fwrite(minified_source, sizeof(char), minified_size, fp);
    fclose(fp);
    free(minified_source);

    if (written_bytes != minified_size) {
        PyErr_SetString(PyExc_OSError, "Error writing TCL file");
        return NULL;
    }

    return Py_None;
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
