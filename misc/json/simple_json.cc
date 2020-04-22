
#include <stdio.h>
#include <iostream>
#include "simple_json.h"
#include "json_tokenize_and_parse.h"

namespace SimpleJson {

// ------------------------ Collector methods ------------------------

SimpleJsonCollector :: SimpleJsonCollector(JsonBodyCallback_t _callback,
                                           void * _callback_arg)
{
    callback = _callback;
    callback_arg = _callback_arg;
    init();
}

SimpleJsonCollector :: ~SimpleJsonCollector(void)
{
}

void
SimpleJsonCollector :: init(void)
{
    contents.clear();
    brace_level = 0;
    in_string = in_escape = false;
}

bool
SimpleJsonCollector :: add_char(char c)
{
    bool ret = false;
    contents += c;
    if (in_string)
    {
        if (in_escape)
            in_escape = false;
        else if (c == '\\')
            in_escape = true;
        else if (c == '"')
            in_string = false;
    }
    else
    {
        if (c == '"')
            in_string = true;
        else if (c == '{')
            brace_level ++;
        else if (c == '}')
        {
            if (--brace_level == 0)
            {
                // complete message!
                ret = true;
                if (callback)
                    callback(callback_arg, contents);
                contents.clear();
            }
        }
    }
    return ret;
}

size_t
SimpleJsonCollector :: add_data(const char *buffer, int len)
{
    size_t ret = 0;

    while (len-- > 0)
    {
        ret++;
        if (add_char(*buffer++) == true)
            break;
    }

    return ret;
}

// ------------------------ parseJson method ------------------------

ObjectProperty * parseJson(const std::string &input)
{
    FILE *f = fmemopen((void*)input.c_str(), input.size(), "r");
    ObjectProperty * ret = json_parser(f);
    fclose(f);
    return ret;
}

// ------------------------ ArrayProperty methods ------------------------

void
ArrayProperty :: resize(size_t newsize)
{
    size_t s = values.size();
    if (newsize > s)
    {
        values.resize(newsize);
        for (size_t ind = s; ind < newsize; ind++)
            values[ind] = NULL;
    }
    else
    {
        for (size_t ind = newsize; ind < s; ind++)
            if (values[ind])
                delete values[ind];
        values.resize(newsize);
    }
}

void
ArrayProperty :: set(size_t ind, Property *value)
{
    if (values[ind])
        delete values[ind];
    values[ind] = value;
}

// ------------------------ ObjectProperty methods ------------------------

Property *
ObjectProperty :: getName(const std::string &n)
{
    for (size_t ind = 0; ind < values.size(); ind++)
    {
        Property * ret = values[ind];
        if (ret && ret->name == n)
            return ret;
    }
    return NULL;
}

// ------------------------ operator<< methods ------------------------

static void
output(std::ostream &str, Property *p)
{
    switch (p->type)
    {
    case Property::INT:      str << p->cast<    IntProperty>();   break;
    case Property::FLOAT:    str << p->cast<  FloatProperty>();   break;
    case Property::STRING:   str << p->cast< StringProperty>();   break;
    case Property::TRINARY:  str << p->cast<TrinaryProperty>();   break;
    case Property::ARRAY:    str << p->cast<  ArrayProperty>();   break;
    case Property::OBJECT:   str << p->cast< ObjectProperty>();   break;
    }
}

std::ostream &operator<<(std::ostream &str, IntProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        str << p->value;
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, FloatProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        str << p->value;
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, StringProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        str << "\"";
        for (size_t ind = 0; ind < p->value.size(); ind++)
        {
            char c = p->value[ind];
            // must escape embedded double quotes to prevent
            // an error in the syntax.
            if (c == '"')
                str << '\\';
            str << c;
        }
        str << "\"";
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, TrinaryProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        switch (p->value)
        {
        case TrinaryProperty::TP_TRUE:  str << "true" ;  break;
        case TrinaryProperty::TP_FALSE: str << "false";  break;
        case TrinaryProperty::TP_NULL:  str << "null" ;  break;
        }
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, ArrayProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        str << "[";
        for (size_t ind = 0; ind < p->size(); ind++)
        {
            Property *p2 = p->get(ind);
            if (p2)
                output(str, p2);
            else
                str << "null";
            if (ind != (p->size()-1))
                str << ",";
        }
        str << "]";
    }
    return str;
}

std::ostream &operator<<(std::ostream &str, ObjectProperty *p)
{
    if (p)
    {
        if (p->name.size() > 0)
            str << "\"" << p->name << "\"" << ":";
        str << "{";
        for (size_t ind = 0; ind < p->size(); ind++)
        {
            Property *p2 = p->get(ind);
            if (p2)
                output(str, p2);
            else
                str << "null";
            if (ind != (p->size()-1))
                str << ",";
        }
        str << "}";
    }
    return str;
}

}; // namespace SimpleJson
