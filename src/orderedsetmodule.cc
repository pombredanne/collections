#include <Python.h>
#include "orderedsetobject.h"

static PyMethodDef module_methods[] = {
    {NULL}  /* Sentinel */
};

#if PY_MAJOR_VERSION > 2
static struct PyModuleDef orderedset_module = {
        PyModuleDef_HEAD_INIT,
        "orderedset",
        NULL,
        -1,
        module_methods,
        NULL,
        NULL,
        NULL,
        NULL
};
#endif

#ifndef PyMODINIT_FUNC  /* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC
#if PY_MAJOR_VERSION > 2
PyInit_orderedset(void)
#else
initorderedset(void)
#endif
{
    PyObject* m = NULL;

    if (PyType_Ready(&PyOrderedSet_Type) < 0)
        goto done;

#if PY_MAJOR_VERSION > 2
    m = PyModule_Create(&orderedset_module);
#else
    m = Py_InitModule("orderedset", module_methods);
#endif

    if (m == NULL)
        goto done;

    Py_INCREF(&PyOrderedSet_Type);
    PyModule_AddObject(m, "orderedset", (PyObject *)&PyOrderedSet_Type);

done:
#if PY_MAJOR_VERSION > 2
    return m;
#else
    return;
#endif
}
