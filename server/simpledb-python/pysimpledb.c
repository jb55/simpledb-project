#include "Python.h"
#include "../simpledb/simpledb.h"

static PyObject* SDBError;
PyMODINIT_FUNC initsimpledb(void);

static PyObject* simpledb_init(PyObject *self, PyObject *args);
static PyObject* simpledb_close(PyObject *self, PyObject *args);
static PyObject* simpledb_update(PyObject *self, PyObject *args);
static PyObject* simpledb_insert(PyObject *self, PyObject *args);
static PyObject* simpledb_find(PyObject *self, PyObject *args);

static void simpledb_raise_error(int err);

static struct sdb_context* g_context = NULL;

static PyMethodDef SDBMethods[] = {
    {"init", simpledb_init, METH_VARARGS, 
    "simpledb.init(db_name) -> Initializes a database"},
    {"update", simpledb_update, METH_VARARGS, 
    "simpledb.update(id, first_name, last_name, date) -> Updates a record in the database"},
    {"insert", simpledb_insert, METH_VARARGS, 
    "simpledb.insert(first_name, last_name, date) -> Inserts a record in the database"},
    {"find", simpledb_find, METH_VARARGS|METH_KEYWORDS, 
    "simpledb.find(id) -> Finds a record in the database"},
    {"close", simpledb_close, METH_NOARGS, 
    "simpledb.close() -> Destroys the database context"},
    {NULL, NULL, 0, NULL}
};

int main(int argc, char *argv[])
{
    /* pass argv[0] to the Python interpreter */
    Py_SetProgramName(argv[0]);

    /* Initialize the Python interpreter. Required. */
    Py_Initialize();

    /* Add a static module */
    initsimpledb();

    return 0;
}

PyMODINIT_FUNC initsimpledb(void)
{
    PyObject* m;

    m = Py_InitModule("simpledb", SDBMethods);
    if (m == NULL)
        return;

    SDBError = PyErr_NewException("simpledb.error", NULL, NULL);
    Py_INCREF(SDBError);
    PyModule_AddObject(m, "error", SDBError);
}

static PyObject* simpledb_update(PyObject *self, PyObject *args)
{
    char* first_name, *last_name;
    time_t date;
    int res;
    unsigned int uid;
    struct sdb_record* record;

    if( !PyArg_ParseTuple(args, "issi", &uid, &first_name, &last_name, &date) )
        return NULL;

    sdb_create_record(first_name, last_name, date, &record);
    res = sdb_update(g_context, uid, record);
    sdb_destroy_record(record);

    if( res != SDB_OK )
    {
        simpledb_raise_error(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* simpledb_insert(PyObject *self, PyObject *args)
{
    char* first_name, *last_name;
    time_t date;
    struct sdb_record* record;
    int res;

    if( !PyArg_ParseTuple(args, "ssi", &first_name, &last_name, &date) )
        return NULL;

    sdb_create_record(first_name, last_name, date, &record);
    res = sdb_insert(g_context, record);
    sdb_destroy_record(record);

    if( res != SDB_OK )
    {
        simpledb_raise_error(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject* simpledb_find(PyObject *self, PyObject *args)
{
    unsigned int uid;
    int res;
    struct sdb_record* record;
    unsigned int offset;
    PyObject* ret;

    if( !PyArg_ParseTuple(args, "i", &uid) )
        return NULL;
    res = sdb_find(g_context, uid, &record, &offset);

    if( res != SDB_OK )
    {
        simpledb_raise_error(res);
        return NULL;
    }

    ret = Py_BuildValue("ssi", record->first_name, record->last_name, record->date);
    sdb_destroy_record(record);

    return ret;
}

static PyObject* simpledb_init(PyObject *self, PyObject *args)
{
    char* db_name;
    int res = SDB_OK;

    if(!PyArg_ParseTuple(args, "s", &db_name))
        return NULL;

    res = sdb_init(db_name, &g_context);
    if( res != SDB_OK )
    {
        simpledb_raise_error(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject* simpledb_close(PyObject *self, PyObject *args)
{
    int res = SDB_OK;

    res = sdb_close(g_context);
    g_context = NULL;

    if( res != SDB_OK )
    {
        simpledb_raise_error(res);
        return NULL;
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static void simpledb_raise_error(int err)
{
    switch(err)
    {
    case SDB_OK:
        return;
    case SDB_RESOURCE_LOCKED:
        PyErr_SetString(SDBError, "The database resource is locked");
        break;
    case SDB_RECORD_NOT_FOUND:
        PyErr_SetString(SDBError, "The specified record was not found");
        break;
    case SDB_NOT_INITIALIZED:
        PyErr_SetString(SDBError, "The database has not been initialized");
        break;
    case SDB_COULD_NOT_OPEN_FILE:
        PyErr_SetString(SDBError, "Could not open file");
        break;
    case SDB_INDEX_UPDATE_ERROR:
        PyErr_SetString(SDBError, "There was an error updating the index, check your syntax and try again.");
        break;
    case SDB_NO_ORPHANS_MATCH:
        PyErr_SetString(SDBError, "Internal orphan error");
        break;
    case SDB_MAX_RECORDS:
        PyErr_SetString(SDBError, "Max records reached");
        break;
    }

    return;
}
