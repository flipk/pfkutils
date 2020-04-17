
#include <vector>
#include <string>

struct ProtoFileEnum; // forward
struct ProtoFileEnumValue
{
    struct ProtoFileEnumValue * next;
    ProtoFileEnum * parent;
    std::string name;
    int value;
    ProtoFileEnumValue(void) { next = NULL; parent = NULL; }
    ~ProtoFileEnumValue(void) { if (next) delete next; }
};

struct ProtoFile; // forward
struct ProtoFileEnum
{
    struct ProtoFileEnum * next;
    ProtoFile * parent;
    std::string name;
    ProtoFileEnumValue * values;
    ProtoFileEnumValue ** values_next;
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
    struct ProtoFileMessageField * next;
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
    struct ProtoFileMessage * next;
    ProtoFile * parent;
    ProtoFileMessageField * fields;
    ProtoFileMessageField ** fields_next;
    std::string name;
    ProtoFileMessage(void) {
        next = NULL; parent = NULL;
        fields = NULL; fields_next = &fields; }
    ~ProtoFileMessage(void) {
        if (next) delete next;
        if (fields) delete fields; }
    void addField(ProtoFileMessageField *f) {
        f->parent = this; *fields_next = f; fields_next = &f->next; }
};

struct ProtoFile
{
    struct ProtoFile * next;
    struct ProtoFile * parent; // if import
    std::string package;
    std::vector<std::string> import_filenames;
    struct ProtoFile * imports;
    struct ProtoFile ** imports_next;
    ProtoFileEnum * enums;
    ProtoFileEnum ** enums_next;
    ProtoFileMessage * messages;
    ProtoFileMessage ** messages_next;
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
    void addImport(struct ProtoFile *pf) {
        pf->parent = this; *imports_next = pf; imports_next = &pf->next; }
    void addEnum(ProtoFileEnum *e) {
        e->parent = this; *enums_next = e; enums_next = &e->next; }
    void addMessage(ProtoFileMessage *m) {
        m->parent = this; *messages_next = m; messages_next = &m->next; }
};

std::ostream &operator<<(std::ostream &strm, const ProtoFile *pf);
std::ostream &operator<<(std::ostream &strm, const ProtoFileMessage *m);
std::ostream &operator<<(std::ostream &strm, const ProtoFileMessageField *mf);
std::ostream &operator<<(std::ostream &strm, const ProtoFileEnum *e);
std::ostream &operator<<(std::ostream &strm, const ProtoFileEnumValue *ev);

void protobuf_json_parser_debug_tokenize(const std::string &fname);
ProtoFile * protobuf_parser(const std::string &fname);

// used by flex to customize the lex function's signature and args
#define YY_DECL int protobuf_json_tokenizer_lex(YYSTYPE *yylval, yyscan_t yyscanner)

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
