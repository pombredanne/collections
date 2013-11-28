#include <Python.h>
#include <boost/tuple/tuple.hpp>
#include "orderedsetobject.h"

#define PyObject_IsIterable(ob) \
    PyObject_HasAttrString(ob, "__iter__")

static int
set_contains(PyOrderedSetObject *self, PyObject *key)
{
    long hash;

    if (!PyString_CheckExact(key) ||
        (hash = ((PyStringObject *) key)->ob_shash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }

    ordered_set_by_hash &hashset = self->oset.get<hash_index>();
    ordered_set_by_hash::iterator it = hashset.find(hash);
    return it != hashset.end();
}

static PyObject *
set_index(PyOrderedSetObject *self, PyObject *key)
{
    long hash;

    if (!PyString_CheckExact(key) ||
        (hash = ((PyStringObject *) key)->ob_shash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1) {
            PyErr_SetString(PyExc_TypeError, "unhashable type");
            return NULL;
        }
    }

    ordered_set_by_hash &hashset = self->oset.get<hash_index>();
    ordered_set_by_hash::iterator it = hashset.find(hash);

    if (it != hashset.end()) {
        ordered_set_by_key &set = self->oset.get<key_index>();
        ordered_set_by_key::iterator it2 = self->oset.project<key_index>(it);
        long n = std::distance(set.begin(), it2);
        return PyInt_FromLong(n);
    }
    PyErr_SetString(PyExc_ValueError, "x is not in set");
    return NULL;
}

PyDoc_STRVAR(index_doc,
"Return index of value.\n"
"Raises ValueError if the value is not present.");

static int
set_add_key(PyOrderedSetObject *self, PyObject *key)
{
    int contains = set_contains(self, key);
    if (contains == -1) {
        // key is not hashable
        return -1;
    }
    else if (contains == 1) {
        // key is exists
        return 0;
    }
    else {
        // key is not exists
        ordered_set_by_key &set = self->oset.get<key_index>();
        set.push_back(ordered_set::value_type(key));
        return 0;
    }
}

static int
set_discard_key(PyOrderedSetObject *self, PyObject *key)
{
    long hash;

    assert (PyOrderedSet_Check(self));
    if (!PyString_CheckExact(key) ||
        (hash = ((PyStringObject *) key)->ob_shash) == -1) {
        hash = PyObject_Hash(key);
        if (hash == -1)
            return -1;
    }

    ordered_set_by_hash &hashset = self->oset.get<hash_index>();
    ordered_set::size_type status = hashset.erase(hash);
    return status > 0;
}

static int
set_clear_internal(PyOrderedSetObject *self)
{
    assert (PyOrderedSet_Check(self));
    self->oset.clear();
    return 0;
}

static void
set_dealloc(PyOrderedSetObject *self)
{
    set_clear_internal(self);
    self->ob_type->tp_free((PyObject *)self);
}

static int
set_print(PyOrderedSetObject *self, FILE *fp, int flags)
{
    const char *emit = "";  /* No separator emitted on first pass */
    const char *separator = ", ";
    int status = Py_ReprEnter((PyObject*)self);

    if (status != 0) {
        if (status < 0)
            return status;
        fprintf(fp, "%s(...)", self->ob_type->tp_name);
        return 0;
    }

    fprintf(fp, "%s([", self->ob_type->tp_name);
    ordered_set_by_key &set = self->oset.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = set.begin(); it < set.end(); it++) {
        ordered_set::value_type entry = *it;
        fputs(emit, fp);
        emit = separator;
        if (PyObject_Print(entry.key, fp, 0) != 0) {
            Py_ReprLeave((PyObject*)self);
            return -1;
        }
    }
    fputs("])", fp);
    Py_ReprLeave((PyObject*)self);
    return 0;
}

static PyObject *
set_repr(PyOrderedSetObject *self)
{
    PyObject *keys, *result=NULL, *listrepr;
    int status = Py_ReprEnter((PyObject*)self);

    if (status != 0) {
        if (status < 0)
            return NULL;
        return PyString_FromFormat("%s(...)", self->ob_type->tp_name);
    }

    keys = PySequence_List((PyObject *)self);
    if (keys == NULL)
        goto done;
    listrepr = PyObject_Repr(keys);
    Py_DECREF(keys);
    if (listrepr == NULL)
        goto done;

    result = PyString_FromFormat("%s(%s)", self->ob_type->tp_name,
                                 PyString_AS_STRING(listrepr));
    Py_DECREF(listrepr);
done:
    Py_ReprLeave((PyObject*)self);
    return result;
}

static Py_ssize_t
set_len(PyOrderedSetObject *self)
{
    return self->oset.size();
}

static PyObject *
set_pop(PyOrderedSetObject *self, PyObject *args)
{
    Py_ssize_t i = -1, len;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "|n:pop", &i))
        return NULL;

    len = set_len(self);
    if (len == 0) {
        /* Special-case most common failure cause */
        PyErr_SetString(PyExc_IndexError, "pop from empty set");
        return NULL;
    }
    if (i < 0)
        i += len;
    if (i < 0 || i >= len) {
        PyErr_SetString(PyExc_IndexError, "pop index out of range");
        return NULL;
    }
    ordered_set_by_key &set = self->oset.get<key_index>();
    v = set[i].key;
    Py_INCREF(v);
    set.erase(set.begin() + i);
    return v;
}

PyDoc_STRVAR(pop_doc,
"Remove and return item at index (default last).\n\
\n\
Raises IndexError if set is empty or index is out of range.");

static int
set_traverse(PyOrderedSetObject *self, visitproc visit, void *arg)
{
    ordered_set_by_key &set = self->oset.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = set.begin(); it < set.end(); it++) {
        ordered_set::value_type entry = *it;
        Py_VISIT(entry.key);
    }
    return 0;
}

static long
set_nohash(PyObject *self)
{
    PyErr_SetString(PyExc_TypeError, "set objects are unhashable");
    return -1;
}

/***** Set iterator type ***********************************************/

typedef struct {
    PyObject_HEAD
    PyOrderedSetObject *si_set; /* Set to NULL when iterator is exhausted */
    Py_ssize_t si_size;
    Py_ssize_t si_pos;
    Py_ssize_t len;
} setiterobject;

static void
setiter_dealloc(setiterobject *si)
{
    Py_XDECREF(si->si_set);
    PyObject_Del(si);
}

static PyObject *
setiter_len(setiterobject *si)
{
    Py_ssize_t len = 0;
    if (si->si_set != NULL && si->si_size == set_len(si->si_set))
        len = si->len;
    return PyInt_FromLong(len);
}

PyDoc_STRVAR(length_hint_doc, "Private method returning an estimate of len(list(it)).");

static PyMethodDef setiter_methods[] = {
    {"__length_hint__", (PyCFunction)setiter_len, METH_NOARGS, length_hint_doc},
    {NULL, NULL} /* sentinel */
};

static PyObject *
setiter_iternext(setiterobject *si)
{
    PyObject *key;
    Py_ssize_t i, mask;
    PyOrderedSetObject *so = si->si_set;

    if (so == NULL)
        return NULL;
    assert (PyOrderedSet_Check(so));

    if (si->si_size != set_len(so)) {
        PyErr_SetString(PyExc_RuntimeError,
                        "Set changed size during iteration");
        si->si_size = -1; /* Make this state sticky */
        return NULL;
    }

    i = si->si_pos;
    assert (i >= 0);
    mask = si->si_size - 1;
    si->si_pos = i + 1;

    if (i > mask) {
        Py_DECREF(so);
        si->si_set = NULL;
        return NULL;
    }

    si->len--;
    ordered_set_by_key &set = si->si_set->oset.get<key_index>();
    key = set[i].key;
    Py_INCREF(key);
    return key;
}

static PyTypeObject PyOrderedSetIter_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /* ob_size */
    "orderedsetiterator",       /* tp_name */
    sizeof(setiterobject),      /* tp_basicsize */
    0,                          /* tp_itemsize */
    /* methods */
    (destructor)setiter_dealloc, /* tp_dealloc */
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
    0,                          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    PyObject_SelfIter,          /* tp_iter */
    (iternextfunc)setiter_iternext, /* tp_iternext */
    setiter_methods,            /* tp_methods */
    0,
};

static PyObject *
set_iter(PyOrderedSetObject *self)
{
    setiterobject *si = PyObject_New(setiterobject, &PyOrderedSetIter_Type);
    if (si == NULL)
        return NULL;
    Py_INCREF(self);
    Py_ssize_t size = set_len(self);
    si->si_set = self;
    si->si_size = size;
    si->si_pos = 0;
    si->len = size;
    return (PyObject *)si;
}

static int
set_update_internal(PyOrderedSetObject *self, PyObject *other)
{
    PyObject *key, *it;

    it = PyObject_GetIter(other);
    if (it == NULL)
        return -1;

//    Py_ssize_t set_size = PyObject_Length(other);
//    ordered_set_by_key &set = self->oset.get<key_index>();
//    set_size += set.size();
//    set.reserve(set_size);

    while ((key = PyIter_Next(it)) != NULL) {
        if (set_add_key(self, key) == -1) {
            Py_DECREF(it);
            Py_DECREF(key);
            return -1;
        }
        Py_DECREF(key);
    }
    Py_DECREF(it);
    if (PyErr_Occurred())
        return -1;
    return 0;
}

static PyObject *
set_update(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    if (set_update_internal(self, other) == -1)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(update_doc, "Update a set with the union of itself and another.");

static PyObject *
make_new_set(PyTypeObject *type, PyObject *iterable)
{
    register PyOrderedSetObject *so = NULL;

    if (type == &PyOrderedSet_Type) {
        so = (PyOrderedSetObject *)type->tp_alloc(type, 0);
        if (so == NULL)
            return NULL;

        ordered_set::ctor_args_list args_list = boost::make_tuple(
            ordered_set_by_key::ctor_args(),
            ordered_set_by_hash::ctor_args());
        ordered_set set(args_list);
        so->oset = set;
    }

    if (iterable != NULL) {
        if (set_update_internal(so, iterable) == -1) {
            Py_DECREF(so);
            return NULL;
        }
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

static PyObject *
set_copy(PyOrderedSetObject *self)
{
    PyOrderedSetObject *so = (PyOrderedSetObject *)make_new_set(self->ob_type, NULL);
    ordered_set new_set(self->oset); // Use copy constructor to create a shallow copy.
    so->oset = new_set;

    return (PyObject *)so;
}

PyDoc_STRVAR(copy_doc, "Return a shallow copy of a set.");

static PyObject *
set_clear(PyOrderedSetObject *self)
{
    set_clear_internal(self);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(clear_doc, "Remove all elements from this set.");

static PyObject *
set_union(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(self) || !PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *result;

    result = set_copy(self);
    if (result == NULL)
        return NULL;
    if (set_update_internal((PyOrderedSetObject *)result, other) == -1)
        return NULL;
    return result;
}

PyDoc_STRVAR(union_doc,
"Return the union of two sets as a new set.\n\
\n\
(i.e. all elements that are in either set.)");

static PyObject *
set_or(PyOrderedSetObject *self, PyObject *other)
{
    return set_union(self, other);
}

static PyObject *
set_ior(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    if (set_update_internal(self, other) == -1)
        return NULL;
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
set_intersection(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(self) || !PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyOrderedSetObject *otherset, *result;

    if (!PyOrderedSet_Check(other)) {
        otherset = (PyOrderedSetObject *)make_new_set(self->ob_type, other);
        if (otherset == NULL)
            return NULL;
        return set_intersection(self, (PyObject *)otherset);
    }

    otherset = (PyOrderedSetObject *)other;
    result = (PyOrderedSetObject *)make_new_set(self->ob_type, NULL);

    ordered_set_by_key &set = self->oset.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = set.begin(); it < set.end(); it++) {
        ordered_set::value_type entry = *it;
        if (set_contains(otherset, entry.key)) {
            set_add_key(result, entry.key);
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    return (PyObject *)result;
}

PyDoc_STRVAR(intersection_doc,
"Return the intersection of two sets as a new set.\n\
\n\
(i.e. all elements that are in both sets.)");

static PyObject *
set_intersection_update(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *tmp;

    tmp = set_intersection(self, other);
    if (tmp == NULL)
        return NULL;
    self->oset = ((PyOrderedSetObject *)tmp)->oset;
    Py_DECREF(tmp);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(intersection_update_doc,
"Update a set with the intersection of itself and another.");

static PyObject *
set_and(PyOrderedSetObject *self, PyObject *other)
{
    return set_intersection(self, other);
}

static PyObject *
set_iand(PyOrderedSetObject *self, PyObject *other)
{
    PyObject *result;

    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }
    result = set_intersection_update(self, other);
    if (result == NULL)
        return NULL;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
set_difference(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(self) || !PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyOrderedSetObject *otherset, *result;

    if (!PyOrderedSet_Check(other)) {
        otherset = (PyOrderedSetObject *)make_new_set(self->ob_type, other);
        if (otherset == NULL)
            return NULL;
        return set_difference(self, (PyObject *)otherset);
    }

    otherset = (PyOrderedSetObject *)other;
    result = (PyOrderedSetObject *)make_new_set(self->ob_type, NULL);

    ordered_set_by_key &aset = self->oset.get<key_index>();
    for (ordered_set_by_key::iterator it = aset.begin(); it < aset.end(); it++) {
        ordered_set::value_type entry = *it;
        if (!set_contains(otherset, entry.key)) {
            set_add_key(result, entry.key);
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    return (PyObject *)result;
}

PyDoc_STRVAR(difference_doc,
"Return the difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in this set but not the other.)");

static PyObject *
set_difference_update(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *tmp;

    tmp = set_difference(self, other);
    if (tmp == NULL)
        return NULL;
    self->oset = ((PyOrderedSetObject *)tmp)->oset;
    Py_DECREF(tmp);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(difference_update_doc,
"Remove all elements of another set from this set.");

static PyObject *
set_sub(PyOrderedSetObject *self, PyObject *other)
{
    return set_difference(self, other);
}


static PyObject *
set_isub(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *result;

    result = set_difference_update(self, other);
    if (result == NULL)
        return NULL;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
set_symmetric_difference(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(self) || !PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyOrderedSetObject *otherset, *result;

    if (!PyOrderedSet_Check(other)) {
        otherset = (PyOrderedSetObject *)make_new_set(self->ob_type, other);
        if (otherset == NULL)
            return NULL;
        return set_symmetric_difference(self, (PyObject *)otherset);
    }

    otherset = (PyOrderedSetObject *)other;
    result = (PyOrderedSetObject *)make_new_set(self->ob_type, NULL);

    ordered_set_by_key &aset = self->oset.get<key_index>();
    for (ordered_set_by_key::iterator it = aset.begin(); it < aset.end(); it++) {
        ordered_set::value_type entry = *it;
        if (!set_contains(otherset, entry.key)) {
            set_add_key(result, entry.key);
        }
    }

    ordered_set_by_key &bset = otherset->oset.get<key_index>();
    for (ordered_set_by_key::iterator it = bset.begin(); it < bset.end(); it++) {
        ordered_set::value_type entry = *it;
        if (!set_contains(self, entry.key)) {
            set_add_key(result, entry.key);
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }
    return (PyObject *)result;
}

PyDoc_STRVAR(symmetric_difference_doc,
"Return the symmetric difference of two sets as a new set.\n\
\n\
(i.e. all elements that are in exactly one of the sets.)");

static PyObject *
set_symmetric_difference_update(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *tmp;

    tmp = set_symmetric_difference(self, other);
    if (tmp == NULL)
        return NULL;
    self->oset = ((PyOrderedSetObject *)tmp)->oset;
    Py_DECREF(tmp);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(symmetric_difference_update_doc,
"Update a set with the symmetric difference of itself and another.");

static PyObject *
set_xor(PyOrderedSetObject *self, PyObject *other)
{
    return set_symmetric_difference(self, other);
}

static PyObject *
set_ixor(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyObject_IsIterable(other)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    PyObject *result;

    result = set_symmetric_difference_update(self, other);
    if (result == NULL)
        return NULL;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
set_issubset(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(other)) {
        PyObject *tmp, *result;
        tmp = make_new_set(&PyOrderedSet_Type, other);
        if (tmp == NULL)
            return NULL;
        result = set_issubset(self, tmp);
        Py_DECREF(tmp);
        return result;
    }
    if (set_len(self) > set_len((PyOrderedSetObject *)other))
        Py_RETURN_FALSE;

    ordered_set_by_key &set = self->oset.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = set.begin(); it < set.end(); it++) {
        ordered_set::value_type entry = *it;
        int rv = set_contains((PyOrderedSetObject *)other, entry.key);
        if (rv == -1)
            return NULL;
        if (!rv)
            Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

PyDoc_STRVAR(issubset_doc, "Report whether another set contains this set.");

static PyObject *
set_issuperset(PyOrderedSetObject *self, PyObject *other)
{
    if (!PyOrderedSet_Check(other)) {
        PyObject *tmp, *result;
        tmp = make_new_set(&PyOrderedSet_Type, other);
        if (tmp == NULL)
            return NULL;
        result = set_issuperset(self, tmp);
        Py_DECREF(tmp);
        return result;
    }
    return set_issubset((PyOrderedSetObject *)other, (PyObject *)self);
}

PyDoc_STRVAR(issuperset_doc, "Report whether this set contains another set.");

static PyObject *
set_richcompare(PyObject *v, PyObject *w, int op)
{
    PyOrderedSetObject *vl, *wl;
    Py_ssize_t i, vlen, wlen;

    if (!PyOrderedSet_Check(v) || !PyOrderedSet_Check(w)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

    vl = (PyOrderedSetObject *)v;
    wl = (PyOrderedSetObject *)w;

    // Shortcut: if the lengths differ, the lists differ
    vlen = set_len(vl);
    wlen = set_len(wl);
    if (vlen != wlen && (op == Py_EQ || op == Py_NE)) {
        PyObject *res;
        if (op == Py_EQ)
            res = Py_False;
        else
            res = Py_True;
        Py_INCREF(res);
        return res;
    }

    // Search for the first index where items are different
    ordered_set_by_key &vset = vl->oset.get<key_index>();
    ordered_set_by_key &wset = wl->oset.get<key_index>();
    for (i = 0; i < vlen && i < wlen; i++) {
        int k = PyObject_RichCompareBool(vset[i].key,
                                         wset[i].key, Py_EQ);
        if (k < 0)
            return NULL;
        if (!k)
            break;
    }

    if (i >= vlen || i >= wlen) {
        /* No more items to compare -- compare sizes */
        int cmp;
        PyObject *res;
        switch (op) {
            case Py_LT: cmp = vlen <  wlen; break;
            case Py_LE: cmp = vlen <= wlen; break;
            case Py_EQ: cmp = vlen == wlen; break;
            case Py_NE: cmp = vlen != wlen; break;
            case Py_GT: cmp = vlen >  wlen; break;
            case Py_GE: cmp = vlen >= wlen; break;
            default: return NULL; /* cannot happen */
        }
        if (cmp)
            res = Py_True;
        else
            res = Py_False;
        Py_INCREF(res);
        return res;
    }

    /* We have an item that differs -- shortcuts for EQ/NE */
    if (op == Py_EQ) {
        Py_INCREF(Py_False);
        return Py_False;
    }
    if (op == Py_NE) {
        Py_INCREF(Py_True);
        return Py_True;
    }

    /* Compare the final item again using the proper operator */
    return PyObject_RichCompare(vset[i].key, wset[i].key, op);
}

//static int
//set_nocmp(PyObject *self, PyObject *other)
//{
//  PyErr_SetString(PyExc_TypeError, "cannot compare sets using cmp()");
//  return -1;
//}

static PyObject *
set_add(PyOrderedSetObject *self, PyObject *key)
{
    if (set_add_key(self, key) == -1)
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(add_doc,
"Add an element to a set.\n\
\n\
This has no effect if the element is already present.");

static PyObject *
set_item(PyOrderedSetObject *self, Py_ssize_t i)
{
    if (i < 0 || i >= set_len(self)) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }
    ordered_set_by_key &set = self->oset.get<key_index>();
    Py_INCREF(set[i].key);
    return set[i].key;
}

static PyObject *
set_slice(PyOrderedSetObject *self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    if (ilow < 0)
        ilow = 0;
    else if (ilow > set_len(self))
        ilow = set_len(self);
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > set_len(self))
        ihigh = set_len(self);
    
    PyOrderedSetObject *so = (PyOrderedSetObject *)make_new_set(self->ob_type, NULL);
    if (so == NULL)
        return NULL;
    
    ordered_set_by_key &set = self->oset.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = set.begin() + ilow; it < set.begin() + ihigh; it++) {
        ordered_set::value_type entry = *it;
        set_add_key(so, entry.key);
    }

    return (PyObject *)so;
}

static int
set_ass_item(PyOrderedSetObject *self, Py_ssize_t i, PyObject *key)
{
    if (i < 0 || i >= set_len(self)) {
        PyErr_SetString(PyExc_IndexError, "list assignment index out of range");
        return -1;
    }

    ordered_set_by_key &set = self->oset.get<key_index>();
    if (key == NULL) {
        // delete item
        set.erase(set.begin() + i);
        return 0;
    }
    else {
        set.replace(set.begin() + i, ordered_set::value_type(key));
        return 0;
    }
}

static int
set_ass_slice(PyOrderedSetObject *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *other)
{
    if (ilow < 0)
        ilow = 0;
    else if (ilow > set_len(self))
        ilow = set_len(self);
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > set_len(self))
        ihigh = set_len(self);

    ordered_set old_set(self->oset);
    set_clear_internal(self);

    ordered_set_by_key &old_keyset = old_set.get<key_index>();
    ordered_set_by_key::iterator it;
    for (it = old_keyset.begin(); it < old_keyset.begin() + ilow; it++) {
        ordered_set::value_type entry = *it;
        set_add_key(self, entry.key);
    }
    set_update_internal(self, other);
    for (it = old_keyset.begin() + ihigh; it < old_keyset.end(); it++) {
        ordered_set::value_type entry = *it;
        set_add_key(self, entry.key);
    }

    return 0;
}

static PyObject *
set_subscript(PyOrderedSetObject* self, PyObject* item)
{
    if (PyIndex_Check(item)) {
        Py_ssize_t i;
        i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += set_len(self);
        return set_item(self, i);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelength, cur, i;

        if (PySlice_GetIndicesEx((PySliceObject*)item, set_len(self),
                                 &start, &stop, &step, &slicelength) < 0) {
            return NULL;
        }

        if (slicelength <= 0) {
            return make_new_set(&PyOrderedSet_Type, NULL);
        }
        else {
            PyOrderedSetObject *so = (PyOrderedSetObject *)make_new_set(&PyOrderedSet_Type, NULL);
            ordered_set_by_key &set = self->oset.get<key_index>();
            for (cur = start, i = 0; i < slicelength; cur += step, i++) {
                set.push_back(ordered_set::value_type(set[cur].key));
            }

            return (PyObject *)so;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError, "list indices must be integers");
        return NULL;
    }
}

static PyObject *
set_remove(PyOrderedSetObject *self, PyObject *key)
{
    int rv = set_discard_key(self, key);
    if (rv == -1) {
        if (!PyOrderedSet_Check(self) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
    } else if (rv == 0) {
        PyErr_SetString(PyExc_ValueError, "orderedset.remove(x): x not in set");
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(remove_doc,
"Remove an element from a set; it must be a member.\n\
\n\
Raises ValueError if the element is not a member.");

static PyObject *
set_discard(PyOrderedSetObject *self, PyObject *key)
{
    int rv = set_discard_key(self, key);
    if (rv == -1) {
        if (!PyOrderedSet_Check(self) || !PyErr_ExceptionMatches(PyExc_TypeError))
            return NULL;
        PyErr_Clear();
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(discard_doc,
"Remove an element from a set if it is a member.\n\
\n\
If the element is not a member, do nothing.");

static PyObject *
set_reduce(PyOrderedSetObject *self)
{
    PyObject *keys = NULL, *args = NULL, *result = NULL, *dict = NULL;

    keys = PySequence_List((PyObject *)self);
    if (keys == NULL)
        goto done;
    args = PyTuple_Pack(1, keys);
    if (args == NULL)
        goto done;
    dict = PyObject_GetAttrString((PyObject *)self, "__dict__");
    if (dict == NULL) {
        PyErr_Clear();
        dict = Py_None;
        Py_INCREF(dict);
    }
    result = PyTuple_Pack(3, self->ob_type, args, dict);
done:
    Py_XDECREF(args);
    Py_XDECREF(keys);
    Py_XDECREF(dict);
    return result;
}

PyDoc_STRVAR(reduce_doc, "Return state information for pickling.");

static int
set_init(PyOrderedSetObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *iterable = NULL;

    if (!PyOrderedSet_Check(self))
        return -1;
    if (!PyArg_UnpackTuple(args, self->ob_type->tp_name, 0, 1, &iterable))
        return -1;

    ordered_set::ctor_args_list args_list = boost::make_tuple(
        ordered_set_by_key::ctor_args(),
        ordered_set_by_hash::ctor_args());
    ordered_set set(args_list);
    self->oset = set;

    set_clear_internal(self);
    if (iterable == NULL)
        return 0;
    return set_update_internal(self, iterable);
}

static PySequenceMethods set_as_sequence = {
    (lenfunc)set_len,           /* sq_length */
    0,                          /* sq_concat */
    0,                          /* sq_repeat */
    (ssizeargfunc)set_item,     /* sq_item */
    (ssizessizeargfunc)set_slice, /* sq_slice */
    (ssizeobjargproc)set_ass_item, /* sq_ass_item */
    (ssizessizeobjargproc)set_ass_slice, /* sq_ass_slice */
    (objobjproc)set_contains,   /* sq_contains */
};

/* orderedset object *************************************************/

static PyMethodDef orderedset_methods[] = {
    {"add", (PyCFunction)set_add, METH_O, add_doc},
    {"clear", (PyCFunction)set_clear, METH_NOARGS, clear_doc},
    {"copy", (PyCFunction)set_copy, METH_NOARGS, copy_doc},
    {"discard", (PyCFunction)set_discard, METH_O, discard_doc},
    {"difference", (PyCFunction)set_difference, METH_O, difference_doc},
    {"difference_update", (PyCFunction)set_difference_update, METH_O, difference_update_doc},
    {"index", (PyCFunction)set_index, METH_O, index_doc},
    {"intersection",(PyCFunction)set_intersection, METH_O, intersection_doc},
    {"intersection_update",(PyCFunction)set_intersection_update, METH_O, intersection_update_doc},
    {"issubset", (PyCFunction)set_issubset, METH_O, issubset_doc},
    {"issuperset", (PyCFunction)set_issuperset, METH_O, issuperset_doc},
    {"pop", (PyCFunction)set_pop, METH_VARARGS, pop_doc},
    {"__reduce__", (PyCFunction)set_reduce, METH_NOARGS, reduce_doc},
    {"remove", (PyCFunction)set_remove, METH_O, remove_doc},
    {"symmetric_difference",(PyCFunction)set_symmetric_difference, METH_O, symmetric_difference_doc},
    {"symmetric_difference_update",(PyCFunction)set_symmetric_difference_update, METH_O, symmetric_difference_update_doc},
    {"union", (PyCFunction)set_union, METH_O, union_doc},
    {"update", (PyCFunction)set_update, METH_O, update_doc},
    {NULL, NULL} /* sentinel */
};

static PyNumberMethods set_as_number = {
    0,                          /* nb_add */
    (binaryfunc)set_sub,        /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_divide */
    0,                          /* nb_remainder */
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* nb_positive */
    0,                          /* nb_absolute */
    0,                          /* nb_nonzero */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    (binaryfunc)set_and,        /* nb_and */
    (binaryfunc)set_xor,        /* nb_xor */
    (binaryfunc)set_or,         /* nb_or */
    0,                          /* nb_coerce */
    0,                          /* nb_int */
    0,                          /* nb_long */
    0,                          /* nb_float */
    0,                          /* nb_oct */
    0,                          /* nb_hex */
    0,                          /* nb_inplace_add */
    (binaryfunc)set_isub,       /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_divide */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    (binaryfunc)set_iand,       /* nb_inplace_and */
    (binaryfunc)set_ixor,       /* nb_inplace_xor */
    (binaryfunc)set_ior,        /* nb_inplace_or */
};

static PyMappingMethods set_as_mapping = {
    (lenfunc)set_len,           /* mp_length */
    (binaryfunc)set_subscript,  /* mp_subscript */
    0                           /* mp_ass_subscript */
};

PyDoc_STRVAR(orderedset_doc,
"orderedset(iterable) --> orderedset object\n\
\n\
Build an ordered collection of unique elements.");

PyTypeObject PyOrderedSet_Type = {
    PyObject_HEAD_INIT(&PyType_Type)
    0,                          /* ob_size */
    "orderedset.orderedset",    /* tp_name */
    sizeof(PyOrderedSetObject), /* tp_basicsize */
    0,                          /* tp_itemsize */
    /* methods */
    (destructor)set_dealloc,    /* tp_dealloc */
    (printfunc)set_print,       /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_compare */
    (reprfunc)set_repr,         /* tp_repr */
    &set_as_number,             /* tp_as_number */
    &set_as_sequence,           /* tp_as_sequence */
    &set_as_mapping,            /* tp_as_mapping */
    set_nohash,                 /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,    /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC, /* tp_flags */
    orderedset_doc,             /* tp_doc */
    (traverseproc)set_traverse, /* tp_traverse */
    (inquiry)set_clear_internal, /* tp_clear */
    set_richcompare,            /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    (getiterfunc)set_iter,      /* tp_iter */
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
    PyObject_GC_Del,            /* tp_free */
};
