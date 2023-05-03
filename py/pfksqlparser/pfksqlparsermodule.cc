
#include "pfksql_tokenize_and_parse.h"

// https://docs.python.org/3/c-api/index.html
// https://docs.python.org/3/c-api/arg.html#c.Py_BuildValue
// https://docs.python.org/3/extending/extending.html

static PyObject *
pfksqlparser_parse(PyObject *self, PyObject *args)
{
    const char * command;
    if (!PyArg_ParseTuple(args, "s", &command))
        return NULL;

    SelectCommand * sc = pfksql_parser(command);
    if (sc)
    {
        PyObject * d = PyDict_New();
        PyObject * v;

//        sc->print();

        PYDICT_SET(d, "table", Py_BuildValue("s", sc->table->c_str()));

        PyObject * fl = PyList_New(sc->field_list->field_list.size());

        int fld_index = 0;
        for (auto fld : sc->field_list->field_list)
        {
            v = Py_BuildValue("s", fld->c_str());
            PyList_SetItem(fl, fld_index++, v);
            // note PyList_SetItem DOES NOT INCREMENT refcount! (????)
        }

        PyDict_SetItemString(d, "field_list", fl);
        // note PyDict_SetItemString increments refcount!
        Py_DECREF(fl);

        PYDICT_SET(d, "expr", sc->expr->make_py_obj());

        delete sc;

        return d;
    }
    // else

    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef PfkSqlParserMethods[] = {
    {"parse",  pfksqlparser_parse, METH_VARARGS,
     "parse an SQL query."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

const char * pfksqlparsermod_doc =
    "this module does sql stuff";

static struct PyModuleDef pfksqlparsermodule = {
    PyModuleDef_HEAD_INIT,
    "pfksqlparser",   /* name of module */
    pfksqlparsermod_doc,     /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    PfkSqlParserMethods
};

PyMODINIT_FUNC
PyInit_pfksqlparser(void)
{
    PyObject * m = PyModule_Create(&pfksqlparsermodule);
    if (m == NULL)
        return NULL;

    // could create exceptions here... PyErr_NewException()

    return m;
}
