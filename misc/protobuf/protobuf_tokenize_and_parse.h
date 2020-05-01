
#include <vector>
#include <string>

struct ProtoFileEnum; // forward
struct ProtoFileEnumValue
{
    ProtoFileEnumValue * next;
    ProtoFileEnum * parent;
    std::string name;
    int value;
    ProtoFileEnumValue(void) { next = NULL; parent = NULL; }
    ~ProtoFileEnumValue(void) { if (next) delete next; }
};

struct ProtoFile; // forward
struct ProtoFileEnum
{
    ProtoFileEnum * next;
    ProtoFile * parent;
    std::string name;
    ProtoFileEnumValue * values;
private:
    ProtoFileEnumValue ** values_next;
public:
    ProtoFileEnum(void) {
        next = NULL; parent = NULL; values = NULL; values_next = &values; }
    ~ProtoFileEnum(void) {
        if (next) delete next;
        if (values) delete values; }
    void addValue(ProtoFileEnumValue *pfev) {
        pfev->parent = this; *values_next = pfev; values_next = &pfev->next; }
};

struct ProtoFileMessage; // forward
struct ProtoFileMessageField
{
    ProtoFileMessageField * next;
    ProtoFileMessage * parent;
    typedef enum { OPTIONAL, REQUIRED, REPEATED } occur_t;
    occur_t occur;
    typedef enum { ENUM, MSG, BOOL, STRING, UNARY } typetype_t;
    typetype_t typetype;
    bool external_package;
    std::string type_package;
    std::string type_name;
    ProtoFileMessage * type_msg; // if typetype == MSG
    ProtoFileEnum * type_enum; // if typetype == ENUM
    // todo : UNARY
    std::string name;
    int number;
    ProtoFileMessageField(void) {
        next = NULL; parent = NULL; external_package = false;
        type_msg = NULL; type_enum = NULL; }
    ~ProtoFileMessageField(void) { if (next) delete next; }
};

struct ProtoFileMessage
{
    ProtoFileMessage * next;
    ProtoFile * parent;
    ProtoFileMessage * parent_msg;
    ProtoFileMessageField * fields;
    ProtoFileMessage * sub_messages;
private:
    ProtoFileMessageField ** fields_next;
    ProtoFileMessage ** sub_messages_next;
public:
    std::string name;
    ProtoFileMessage(void) {
        next = NULL; parent = NULL; parent_msg = NULL;
        fields = NULL; fields_next = &fields;
        sub_messages = NULL; sub_messages_next = &sub_messages; }
    ~ProtoFileMessage(void) {
        if (next) delete next;
        if (fields) delete fields; }
    void addField(ProtoFileMessageField *f) {
        f->parent = this; *fields_next = f; fields_next = &f->next; }
    void addSubMessage(ProtoFileMessage *m) {
        m->parent_msg = this; *sub_messages_next = m;
        sub_messages_next = &m->next; }
    void setParent(ProtoFile *f) {
        parent = f;
        for (ProtoFileMessage *m = sub_messages; m; m = m->next)
            m->parent = f;
    }
    std::string fullname(void) {
        std::string ret;
        if (parent_msg)
            ret = parent_msg->fullname() + "_";
        ret += name;
        return ret;
    }
};

struct ProtoFile
{
    ProtoFile * next;
    ProtoFile * parent; // if import
    std::string package;
    std::string filename;
    std::vector<std::string> import_filenames;
    ProtoFile * imports;
    ProtoFileEnum * enums;
    ProtoFileMessage * messages;
private:
    ProtoFile ** imports_next;
    ProtoFileEnum ** enums_next;
    ProtoFileMessage ** messages_next;
public:
    ProtoFile(void) {
        next = parent = NULL;
        imports = NULL;
        imports_next = &imports;
        enums = NULL;
        enums_next = &enums;
        messages = NULL;
        messages_next = &messages;
    }
    ~ProtoFile(void) {
        if (next) delete next;
        if (imports) delete imports;
        if (enums) delete enums;
        if (messages) delete messages;
    }
    void addImport(ProtoFile *pf) {
        pf->parent = this; *imports_next = pf; imports_next = &pf->next; }
    void addEnum(ProtoFileEnum *e) {
        e->parent = this; *enums_next = e; enums_next = &e->next; }
    void addMessage(ProtoFileMessage *m) {
        m->setParent(this); *messages_next = m; messages_next = &m->next; }
};

void protobuf_parser_debug_tokenize(const std::string &fname);
ProtoFile * protobuf_parser(const std::string &fname,
                            const std::vector<std::string> *searchPath);

std::ostream &operator<<(std::ostream &strm, const ProtoFile *pf);
std::ostream &operator<<(std::ostream &strm, const ProtoFileMessage *m);
std::ostream &operator<<(std::ostream &strm, const ProtoFileMessageField *mf);
std::ostream &operator<<(std::ostream &strm, const ProtoFileEnum *e);
std::ostream &operator<<(std::ostream &strm, const ProtoFileEnumValue *ev);

#ifdef __SIMPLE_PROTOBUF_INTERNAL__
// used by flex to customize the lex function's signature and args
#define YY_DECL int protobuf_tokenizer_lex(YYSTYPE *yylval, yyscan_t yyscanner)
#endif /* __SIMPLE_PROTOBUF_INTERNAL__ */

#if 0 // useful for testing the scanner standalone

typedef struct {
    std::string * word_value;
    int int_value;
} YYSTYPE;

#define TOKEN_LIST                              \
    TOKEN_ITEM(COMMENT)                         \
    TOKEN_ITEM(STRING)                          \
    TOKEN_ITEM(WORD)                            \
    TOKEN_ITEM(OPENBRACE)                       \
    TOKEN_ITEM(CLOSEBRACE)                      \
    TOKEN_ITEM(SEMICOLON)                       \
    TOKEN_ITEM(INT)                             \
    TOKEN_ITEM(EQUAL)                           \

enum token_list {
    ____unused_to_make_list_start_at_255 = 254,
#define TOKEN_ITEM(x)  TOK_##x ,
    TOKEN_LIST
#undef TOKEN_ITEM
    NUM_TOKENS
};

#endif // useful for testing scanner standalone
