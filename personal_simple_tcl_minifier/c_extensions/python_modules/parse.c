#define Py_LIMITED_API 0x030c0000
#define PY_SSIZE_T_CLEAN

#include "../includes/parse.h"

#include "../includes/c_optimizations.h"

#include <Python.h>
#include <stdlib.h>

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

static PyMethodDef parse_methods[] = {
    {"tcl_minify", Py_tcl_minify, METH_O, NULL},
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
