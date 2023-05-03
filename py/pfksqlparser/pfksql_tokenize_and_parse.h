/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#define PY_SSIZE_T_CLEAN
#include "python3.11/Python.h"
#include <string>
#include <vector>

// NOTE PyDict_SetItem increments refcount
//      while PyList_SetItem does not!
//      dafuq, over.
#define PYDICT_SET(d, name, obj)          \
    do {                                  \
        PyObject * o = obj;               \
        PyDict_SetItemString(d, name, o); \
        Py_DECREF(o);                     \
    } while (0)

enum Conjunction {
    CONJ_AND, CONJ_OR
};
const char * conj_to_str(Conjunction);

enum Operator {
    OP_LT, OP_GT, OP_LTEQ, OP_GTEQ, OP_EQ, OP_NEQ, OP_LIKE, OP_REGEX
};
const char * op_to_str(Operator);

struct FieldList {
    ~FieldList(void);
    std::vector<std::string*> field_list;
    void print(void);
};

struct Expr {
    enum ExprType {
        EXPR_COMPARE_STR,
        EXPR_COMPARE_NUM,
        EXPR_COMPARE_BOOL,
        EXPR_EXISTS,
        EXPR_CONJ
    };
    Expr(ExprType t) : type(t) { }
    virtual ~Expr(void) { }
    ExprType type;
    virtual void print(int indent = 1) = 0;
    virtual PyObject * make_py_obj(void) = 0;
    const char * tabs(int indent);
};

struct ExprCompareStr : public Expr {
    ExprCompareStr(void) : Expr(EXPR_COMPARE_STR) { }
    ~ExprCompareStr(void) override;
    std::string * field;
    Operator op;
    std::string * str;
    void print(int indent);
    PyObject * make_py_obj(void);
};

struct ExprCompareNum : public Expr {
    ExprCompareNum(void) : Expr(EXPR_COMPARE_NUM) { }
    ~ExprCompareNum(void) override;
    std::string * field;
    Operator op;
    int64_t  val;
    void print(int indent);
    PyObject * make_py_obj(void);
};

struct ExprCompareBool : public Expr {
    ExprCompareBool(void) : Expr(EXPR_COMPARE_BOOL) { }
    ~ExprCompareBool(void) override;
    std::string * field;
    Operator op;
    bool  val;
    void print(int indent);
    PyObject * make_py_obj(void);
};

struct ExprExists : public Expr {
    ExprExists(void) : Expr(EXPR_EXISTS) { }
    ~ExprExists(void) override;
    std::string * field;
    bool exist;
    void print(int indent);
    PyObject * make_py_obj(void);
};

struct ExprConj : public Expr {
    ExprConj(void) : Expr(EXPR_CONJ) { }
    ~ExprConj(void) override;
    Expr * left;
    Conjunction conj;
    Expr * right;
    void print(int indent);
    PyObject * make_py_obj(void);
};

struct SelectCommand {
    SelectCommand(void);
    ~SelectCommand(void);
    std::string * table;
    FieldList * field_list;
    Expr * expr;
    bool error;
    void print(void);
};

void pfksql_parser_debug_tokenize(const char * cmd);
SelectCommand * pfksql_parser(const char * cmd);

#ifdef __PFKSQL_PARSE_INTERNAL__
// used by flex to customize the lex function's signature and args
#define YY_DECL int pfksql_tokenizer_lex( PFK_SQL_PARSER_STYPE *yylval, yyscan_t yyscanner)
#endif

