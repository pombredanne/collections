#ifndef orderedset_orderedsetobject_h
#define orderedset_orderedsetobject_h

#include <vector>
#include <Python.h>
#include <boost/unordered_set.hpp>

using namespace ::std;
using namespace ::boost::unordered;

struct internal_set_entry {
    PyObject *key;

    internal_set_entry(PyObject *key)
    {
        Py_INCREF(key);
        this->key = key;
    }

    internal_set_entry(internal_set_entry const & x)
    {
        Py_INCREF(x.key);
        this->key = x.key;
    }

    ~internal_set_entry()
    {
        Py_DECREF(key);
    }

    bool operator==(const internal_set_entry& other) const
    {
        int i = PyObject_RichCompareBool(key, other.key, Py_EQ);
        return i > 0;
    }
};

class py_hash {
public:
    std::size_t operator()(const internal_set_entry & val) const
    {
        return PyObject_Hash(val.key);
    }
};

struct py_equals : std::binary_function<const internal_set_entry&, const internal_set_entry&, bool> {
    bool operator()(first_argument_type lhs, second_argument_type rhs) const
    {
        int i = PyObject_RichCompareBool(lhs.key, rhs.key, Py_EQ);
        return i > 0;
    }
};

typedef unordered_set<internal_set_entry, py_hash, py_equals> internal_set;
typedef vector<internal_set_entry> internal_vector;

typedef struct _orderedsetobject {
    PyObject_HEAD

    internal_set oset;
    internal_vector olist;
} PyOrderedSetObject;

PyAPI_DATA(PyTypeObject) PyOrderedSet_Type;

#define PyOrderedSet_Check(ob) \
    (Py_TYPE(ob) == &PyOrderedSet_Type || \
    PyType_IsSubtype(Py_TYPE(ob), &PyOrderedSet_Type))

#endif
