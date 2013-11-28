#ifndef orderedset_orderedsetobject_h
#define orderedset_orderedsetobject_h

#include <Python.h>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/mem_fun.hpp>

#ifdef DEBUG
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

using namespace ::boost;
using namespace ::boost::multi_index;

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

    bool operator<(const ordered_set_entry& other) const
    {
        int i = PyObject_RichCompareBool(key, other.key, Py_LT);
        return i > 0;
    }

    long hash() const
    {
        return PyObject_Hash(key);
    }
};

struct key_index{};
struct hash_index{};

typedef multi_index_container<
    ordered_set_entry,
    indexed_by<
        random_access<
            tag<key_index>
        >,

        // hash by ordered_set_entry::hash
        hashed_unique<
            tag<hash_index>,
            const_mem_fun<ordered_set_entry, long, &ordered_set_entry::hash>
        >
    >
> ordered_set;

typedef ordered_set::index<key_index>::type ordered_set_by_key;
typedef ordered_set::index<hash_index>::type ordered_set_by_hash;

typedef struct _orderedsetobject {
    PyObject_HEAD

    ordered_set oset;
} PyOrderedSetObject;

PyAPI_DATA(PyTypeObject) PyOrderedSet_Type;

#define PyOrderedSet_Check(ob) \
    (Py_TYPE(ob) == &PyOrderedSet_Type || \
    PyType_IsSubtype(Py_TYPE(ob), &PyOrderedSet_Type))

#endif
