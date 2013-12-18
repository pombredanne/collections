#ifndef orderedset_orderedsetobject_h
#define orderedset_orderedsetobject_h

#include <Python.h>
#include <boost/unordered_set.hpp>

using namespace ::boost;
using namespace ::boost::unordered;

struct ordered_set_entry {
    PyObject *key;

    ordered_set_entry(PyObject *key)
    {
        Py_INCREF(key);
        this->key = key;
    }

    ordered_set_entry(ordered_set_entry const & x)
    {
        Py_INCREF(x.key);
        this->key = x.key;
    }

    ~ordered_set_entry()
    {
        Py_DECREF(key);
    }
};

class py_hash {
public:
    std::size_t operator()(const ordered_set_entry & val) const
    {
        return PyObject_Hash(val.key);
    }
};

struct py_equals : std::binary_function<const ordered_set_entry&, const ordered_set_entry&, bool> {
    bool operator()(first_argument_type lhs, second_argument_type rhs) const
    {
        int i = PyObject_RichCompareBool(lhs.key, rhs.key, Py_EQ);
        return i > 0;
    }
};

typedef unordered_set<ordered_set_entry, py_hash, py_equals> ordered_set;

typedef struct _orderedsetobject {
    PyObject_HEAD

    ordered_set oset;
} PyOrderedSetObject;

PyAPI_DATA(PyTypeObject) PyOrderedSet_Type;

#define PyOrderedSet_Check(ob) \
    (Py_TYPE(ob) == &PyOrderedSet_Type || \
    PyType_IsSubtype(Py_TYPE(ob), &PyOrderedSet_Type))

#endif
