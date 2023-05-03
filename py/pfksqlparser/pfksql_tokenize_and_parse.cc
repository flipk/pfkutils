
#include "pfksql_tokenize_and_parse.h"

const char * conj_to_str(Conjunction conj)
{
    switch (conj)
    {
    case CONJ_AND: return "and";
    case CONJ_OR:  return "or";
    }
    return "??";
}

const char * op_to_str(Operator op)
{
    switch (op)
    {
    case OP_LT:    return "lt";
    case OP_GT:    return "gt";
    case OP_LTEQ:  return "lteq";
    case OP_GTEQ:  return "gteq";
    case OP_EQ:    return "eq";
    case OP_NEQ:   return "neq";
    case OP_LIKE:  return "like";
    case OP_REGEX: return "regex";
    }
    return "??";
}

FieldList :: ~FieldList(void)
{
    for (auto s : field_list)
        delete s;
}

void FieldList :: print(void)
{
    printf("\tfields: ");
    bool first = true;
    for (auto s : field_list)
    {
        printf("%s%s", first ? "" : ",", s->c_str());
        first = false;
    }
    printf("\n");
}

ExprCompareStr :: ~ExprCompareStr(void)
{
    delete field;
    delete str;
}

void ExprCompareStr :: print(int indent)
{
    printf("%scompare : %s %s %s\n",
           tabs(indent),
           field->c_str(), op_to_str(op), str->c_str());
}

PyObject * ExprCompareStr :: make_py_obj(void)
{
    PyObject * comp = PyDict_New();
    PYDICT_SET(comp, "type",
               Py_BuildValue("s", "str"));
    PYDICT_SET(comp, "field",
               Py_BuildValue("s", field->c_str()));
    PYDICT_SET(comp, "op",
               Py_BuildValue("s", op_to_str(op)));
    PYDICT_SET(comp, "value",
               Py_BuildValue("s", str->c_str()));

    PyObject * ret = PyDict_New();
    PYDICT_SET(ret, "type", Py_BuildValue("s", "compare"));
    PYDICT_SET(ret, "compare", comp);
    return ret;
}

ExprCompareNum :: ~ExprCompareNum(void)
{
    delete field;
}

void ExprCompareNum :: print(int indent)
{
    printf("%scompare : %s %s %d\n",
           tabs(indent),
           field->c_str(), op_to_str(op), val);
}

PyObject * ExprCompareNum :: make_py_obj(void)
{
    PyObject * comp = PyDict_New();
    PYDICT_SET(comp, "type",
               Py_BuildValue("s", "num"));
    PYDICT_SET(comp, "field",
               Py_BuildValue("s", field->c_str()));
    PYDICT_SET(comp, "op",
               Py_BuildValue("s", op_to_str(op)));
    PYDICT_SET(comp, "value",
               Py_BuildValue("i", val));

    PyObject * ret = PyDict_New();
    PYDICT_SET(ret, "type", Py_BuildValue("s", "compare"));
    PYDICT_SET(ret, "compare", comp);
    return ret;
}

ExprCompareBool :: ~ExprCompareBool(void)
{
    delete field;
}

void ExprCompareBool :: print(int indent)
{
    printf("%scompare : %s %s %s\n",
           tabs(indent),
           field->c_str(), op_to_str(op), val ? "true" : "false");
}

PyObject * ExprCompareBool :: make_py_obj(void)
{
    PyObject * comp = PyDict_New();
    PYDICT_SET(comp, "type",
               Py_BuildValue("s", "bool"));
    PYDICT_SET(comp, "field",
               Py_BuildValue("s", field->c_str()));
    PYDICT_SET(comp, "op",
               Py_BuildValue("s", op_to_str(op)));

    // NOTE do NOT use DECREF on a boolean type, these are static.
    PyDict_SetItemString(comp, "value",
                         val ? Py_True : Py_False);

    PyObject * ret = PyDict_New();
    PYDICT_SET(ret, "type", Py_BuildValue("s", "compare"));
    PYDICT_SET(ret, "compare", comp);
    return ret;
}

ExprExists :: ~ExprExists(void)
{
    delete field;
}

void ExprExists :: print(int indent)
{
    printf("%s%sexists: %s\n",
           tabs(indent), exist ? "" : "not_",
           field->c_str());
}

PyObject * ExprExists :: make_py_obj(void)
{
    PyObject * comp = PyDict_New();
    PYDICT_SET(comp, "type",
               Py_BuildValue("s", "exist"));
    PYDICT_SET(comp, "field",
               Py_BuildValue("s", field->c_str()));

    // NOTE do NOT use DECREF on a boolean type, these are static.
    PyDict_SetItemString(comp, "value",
                         exist ? Py_True : Py_False);

    PyObject * ret = PyDict_New();
    PYDICT_SET(ret, "type", Py_BuildValue("s", "exist"));
    PYDICT_SET(ret, "exist", comp);
    return ret;
}

ExprConj :: ~ExprConj(void)
{
    delete left;
    delete right;
}

void ExprConj :: print(int indent)
{
    printf("%sconjunction (%s):\n",
           tabs(indent), conj_to_str(conj));
    left->print(indent+1);
    right->print(indent+1);
}

PyObject * ExprConj :: make_py_obj(void)
{
    PyObject * dconj = PyDict_New();

    PYDICT_SET(dconj, "left", left->make_py_obj());
    PYDICT_SET(dconj, "conj",
               Py_BuildValue("s", conj_to_str(conj)));
    PYDICT_SET(dconj, "right", right->make_py_obj());

    PyObject * ret = PyDict_New();
    PYDICT_SET(ret, "type", Py_BuildValue("s", "conj"));
    PYDICT_SET(ret, "conj", dconj);
    return ret;
}

const char * Expr :: tabs(int indent)
{
    const char * tabs = "\t\t\t\t\t\t\t\t\t\t";
    return tabs + (10-indent);
}        

SelectCommand :: SelectCommand(void)
{
    table = NULL;
    field_list = NULL;
    expr = NULL;
    error = false;
}

SelectCommand :: ~SelectCommand(void)
{
    if (table)
        delete table;
    if (field_list)
        delete field_list;
    if (expr)
        delete expr;
//    printf("~SelectCommand\n");
}

void SelectCommand :: print(void)
{
    printf("SelectCommand contents:\n");
    if (table)
        printf("\ttable : %s\n", table->c_str());
    if (field_list)
        field_list->print();
    if (expr)
        expr->print();
}
