#define Py_LIMITED_API 0x030c0000
#define PY_SSIZE_T_CLEAN

#include <Python.h>

#include <stdlib.h>

#include "../includes/c_optimizations.h"
#include "../includes/parse.h"

static PyObject *Py_tcl_minify(PyObject *self, PyObject *arg)
{
    Py_ssize_t size;
    const char *source = PyUnicode_AsUTF8AndSize(arg, &size);
    if (unlikely(source == NULL))
    {
        return NULL;
    }

    char *minified_source = tcl_minify(source, size);

    PyObject *out = PyUnicode_FromString(minified_source);

    free(minified_source);

    return out;
}

static PyObject *Py_tcl_minify_to_file(PyObject *self, PyObject *args)
{
    Py_ssize_t size;
    const char *source;
    const char *path;

    if (unlikely(!PyArg_ParseTuple(args, "s#s", &source, &size, &path)))
    {
        return NULL;
    }

    char *minified_source = tcl_minify(source, size);

    FILE *fp = fopen("file.txt", "w");
    if (fp == NULL)
    {
        PyErr_SetString(PyExc_OSError, "Failed to open file");
        return NULL;
    }

    fprintf(fp, minified_source);
    free(minified_source);

    return Py_None;
}

static PyMethodDef parse_methods[] = {
    {"tcl_minify", Py_tcl_minify, METH_O, NULL},
    {"tcl_minify_to_file", Py_tcl_minify_to_file, METH_VARARGS, NULL},
    {NULL, NULL, 0, NULL}};

static int parse_exec(PyObject *Py_UNUSED(module))
{
    return 0;
}

static PyModuleDef_Slot parse_slots[] = {
    {Py_mod_exec, parse_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#ifdef Py_GIL_DISABLED
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
#endif
    {0, NULL}};

static struct PyModuleDef parse_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "parse",
    .m_size = 0,
    .m_methods = parse_methods,
    .m_slots = parse_slots};

PyMODINIT_FUNC PyInit_parse(void)
{
    return PyModuleDef_Init(&parse_module);
}
