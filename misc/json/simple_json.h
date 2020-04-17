#ifndef __SIMPLE_JSON_H__
#define __SIMPLE_JSON_H__

#include <vector>
#include <string>

namespace SimpleJson {

class SimpleJsonCollector
{
public:
    typedef void (*JsonBodyCallback_t)(void *arg,
                                       const std::string &object);
private:
    JsonBodyCallback_t callback;
    void * callback_arg;
    std::string contents;
    int brace_level;
    bool in_string;
    bool in_escape;
public:
    SimpleJsonCollector(JsonBodyCallback_t callback,
                        void * callback_arg);
    ~SimpleJsonCollector(void);
    void init(void);
    bool add_char(char c);
    size_t add_data(const char *buffer, int len);
    size_t add_data(const std::string &buf) {
        return add_data((const char*)buf.c_str(), (int) buf.size());
    }
};

struct IntProperty;
struct FloatProperty;
struct StringProperty;
struct TrinaryProperty;
struct ArrayProperty;
struct ObjectProperty;

ObjectProperty *parseJson(const std::string &input);
// to output a JSON message, just use "operator<<"

struct Property
{
    typedef enum { INT, FLOAT, STRING, TRINARY, ARRAY, OBJECT } propertyType;
    Property(propertyType _t) : type(_t) { }
    virtual ~Property(void) { }
    propertyType type;
    std::string name;
    template <class T> T * cast(void) {
        return (type == T::TYPE) ? (T*)this : NULL;
    }
};

struct IntProperty : public Property
{
    static const propertyType TYPE = INT;
    int value;
    IntProperty(int _value = 0) : Property(TYPE), value(_value) { }
};

struct FloatProperty : public Property
{
    static const propertyType TYPE = FLOAT;
    double value;
    FloatProperty(double _value = 0.0) : Property(TYPE), value(_value) { }
};

struct StringProperty : public Property
{
    static const propertyType TYPE = STRING;
    std::string value;
    StringProperty(const std::string &_value = "")
        : Property(TYPE), value(_value) { }
};

struct TrinaryProperty : public Property
{
    static const propertyType TYPE = TRINARY;
    typedef enum { TP_TRUE, TP_FALSE, TP_NULL } value_t;
    value_t value;
    TrinaryProperty(value_t _v) : Property(TYPE), value(_v) { }
};

class ArrayProperty : public Property
{
protected: // ObjectProperty reuses this
    std::vector<Property*> values; // name only used if Object
    ArrayProperty(propertyType _t) : Property(_t) { } // for Object
public:
    static const propertyType TYPE = ARRAY;
    ArrayProperty(void) : Property(TYPE) { }
    size_t size(void) const { return values.size(); }
    void resize(size_t newsize);
    // note 'name' unused for type = ARRAY, but used for type = OBJECT
    Property *get(size_t ind) { return values[ind]; }
    void set(size_t ind, Property *value);
    void push_back(Property *value) { values.push_back(value); }
};

class ObjectProperty : public ArrayProperty
{
public:
    static const propertyType TYPE = OBJECT;
// override array's setting but inherit everything else
    ObjectProperty(void) : ArrayProperty(TYPE) { }
};

std::ostream &operator<<(std::ostream &, IntProperty *);
std::ostream &operator<<(std::ostream &, FloatProperty *);
std::ostream &operator<<(std::ostream &, StringProperty *);
std::ostream &operator<<(std::ostream &, TrinaryProperty *);
std::ostream &operator<<(std::ostream &, ArrayProperty *);
std::ostream &operator<<(std::ostream &, ObjectProperty *);

}; // namespace SimpleJson

#endif /* __SIMPLE_JSON_H__ */
