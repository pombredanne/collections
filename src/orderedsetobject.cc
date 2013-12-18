#include <Python.h>
#include "orderedsetobject.h"

#define PyObject_IsIterable(ob) \
    PyObject_HasAttrString(ob, "__iter__")

#if PY_MAJOR_VERSION > 2
#define PyString_FromFormat PyUnicode_FromFormat
#define PyString_AS_STRING PyUnicode_AS_UNICODE
#endif
#ifndef Py_TYPE
#define Py_TYPE(ob) (((PyObject*)(ob))->ob_type)
#endif
#ifndef PyVarObject_HEAD_INIT
#define PyVarObject_HEAD_INIT(type, size) \
    PyObject_HEAD_INIT(type) size,
#endif

//static int
//set_contains(PyOrderedSetObject *self, PyObject *item)
//{
//    ordered_set::key_type key(item);
//    ordered_set::iterator it = self->oset.find(key);
//    return it != self->oset.end();
//}

static PyObject *
set_add(PyOrderedSetObject *self, PyObject *item)
{
    ordered_set::key_type key(item);
    self->oset.insert(key);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc,
"Add an element to a set.\n\
\n\
This has no effect if the element is already present.");

static PyObject *
set_remove(PyOrderedSetObject *self, PyObject *item)
{
    ordered_set::key_type key(item);
    self->oset.erase(key);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_doc,
"Remove an element from a set; it must be a member.\n\
\n\
             Raises ValueError if the element is not a member.");

static PyObject *
set_len(PyOrderedSetObject *self)
{
    ordered_set::size_type size = self->oset.size();
    return PyLong_FromLong(size);
}

/* orderedset object *************************************************/

static PyMethodDef orderedset_methods[] = {
    {"add", (PyCFunction)set_add, METH_O, add_doc},
    {"remove", (PyCFunction)set_remove, METH_O, remove_doc},
    {"__len__", (PyCFunction)set_len, METH_NOARGS, ""},
    {NULL, NULL} /* sentinel */
};

PyDoc_STRVAR(orderedset_doc,
"orderedset(iterable) --> orderedset object\n\
\n\
Build an ordered collection of unique elements.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
    register PyOrderedSetObject *so = NULL;
    
    if (type == &PyOrderedSet_Type) {
        so = (PyOrderedSetObject *)type->tp_alloc(type, 0);
        if (so == NULL)
            return NULL;

        ordered_set set;
        so->oset = set;
    }
    
    return (PyObject *)so;
}

static PyObject *
set_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    if (type == &PySet_Type && !_PyArg_NoKeywords("orderedset()", kwds))
        return NULL;
    
    return make_new_set(type, NULL);
}

static int
set_init(PyOrderedSetObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;
    
    if (!PyOrderedSet_Check(self))
        return -1;
    if (!PyArg_UnpackTuple(args, self->ob_type->tp_name, 0, 1, &iterable))
        return -1;

    ordered_set set;
    self->oset = set;

    return 0;
}

PyTypeObject PyOrderedSet_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "orderedset.orderedset",    /* tp_name */
    sizeof(PyOrderedSetObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    /* methods */
    0,                          /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,    /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    orderedset_doc,             /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    orderedset_methods,         /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)set_init,         /* tp_init */
    PyType_GenericAlloc,        /* tp_alloc */
    set_new,                    /* tp_new */
    0,                          /* tp_free */
};
