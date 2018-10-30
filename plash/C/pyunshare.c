#include "unshare.c"
#include <Python.h>

static PyObject* pyunshare(PyObject *self, PyObject *args) {
    printf("UNSHARE CALL HERE!\n");
    Py_RETURN_NONE;
}

static PyMethodDef unshare_methods[] = { 
    {   
        "unshare", pyunshare, METH_NOARGS,
        "unshare process resources and map user id"
    },  
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef unshare_definition = { 
    PyModuleDef_HEAD_INIT,
    "unshare",
    "This modules provides the unshare() function call.",
    -1, 
    unshare_methods
};

PyMODINIT_FUNC PyInit_unshare(void) {
    Py_Initialize();
    return PyModule_Create(&unshare_definition);
}
