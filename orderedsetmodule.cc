#include <Python/Python.h>
#include "orderedsetobject.h"

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif
PyMODINIT_FUNC
initorderedset(void)
{
    PyObject* m;

    if (PyType_Ready(&PyOrderedSet_Type) < 0)
        return;

    m = Py_InitModule("orderedset", module_methods);

    if (m == NULL)
        return;

    Py_INCREF(&PyOrderedSet_Type);
    PyModule_AddObject(m, "orderedset", (PyObject *)&PyOrderedSet_Type);
}
