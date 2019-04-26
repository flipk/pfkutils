
#include "myXml.h"
#include <iostream>
#include <stdio.h>

using namespace std;

static inline void
xml_escape_special(string &out, const string &in)
{
    for (size_t ind = 0; ind < in.size(); ind++)
    {
        char c = in[ind];
        switch (c)
        {
        case '<' : out += "&lt;"  ; break;
        case '>' : out += "&gt;"  ; break;
        case '&' : out += "&amp;" ; break;
        case '\'': out += "&apos;"; break;
        case '"' : out += "&quot;"; break;
        default:   out += c;
        }
    }
}

static inline bool
is_whitespace(char c)
{
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        return true;
    return false;
}

static inline bool
is_xmlchar(char c, bool notfirst)
{
    if (c >= 'a' && c <= 'z')
        return true;
    if (c >= 'A' && c <= 'Z')
        return true;
    if (notfirst)
    {
        if (c >= '0' && c <= '9')
            return true;
        if (c == '_' || c == ':')
            return true;
    }
    return false;
}

static inline bool
validate_xmlness(const std::string &name)
{
    for (size_t ind = 0; ind < name.size(); ind++)
    {
        char c = name[ind];
        if (!is_xmlchar(c, ind > 0))
            return false;
    }
    return true;
}

struct replace_patterns_t {
    const string before;
    const string after;
};
const replace_patterns_t replace_patterns[] = {
    { "&lt;", "<" },
    { "&gt;", ">" },
    { "&amp;", "&" },
    { "&apos;", "'" },
    { "&quot;", "\"" },
    { "", "" }
};

static inline void
unescape(std::string &s)
{
    bool found = false;
    while (1)
    {
        found = false;
        size_t pos;
        const replace_patterns_t *rp;

        for (rp = replace_patterns;
             rp->before.size() > 0;
             rp++)
        {
            pos = s.find(rp->before);
            if (pos != string::npos)
            {
                found = true;
                break;
            }
        }
        if (found == false)
            break;
        s.erase(pos, rp->before.size());
        s.insert(pos, rp->after);
    }
}

size_t
MyXmlNode :: parse(const string &xml, size_t start_pos)
{
    init();
    size_t tag_start = xml.find_first_of('<', start_pos);
    if (tag_start != start_pos)
    {
        text = xml.substr(start_pos,tag_start - start_pos);
        unescape(text);
        if (tag_start == string::npos)
            return xml.size();
        return tag_start;
    }

    size_t tag_name_end = xml.find_first_of(" \r\n\t/>", tag_start);
    if (tag_name_end == string::npos)
        return 0;
    name = xml.substr(start_pos+1,tag_name_end-start_pos-1);
    if (validate_xmlness(name) == false)
        return 0;

    bool selfclosed = false;

    size_t tag_end = tag_name_end;
    if (xml[tag_end] != '>')
    {
        tag_end = xml.find_first_of('>', tag_name_end);
        if (tag_end == string::npos)
            return 0;

        if (xml[tag_end-1] == '/')
        {
            selfclosed = true;
            tag_end--;
        }

        enum { st_ws, st_attname, st_attbody } st = st_ws;
        string attname, attbody;
        for (size_t p = tag_name_end + 1; p < tag_end; p++)
        {
            char c = xml[p];
            switch (st)
            {
            case st_ws:
                if (is_xmlchar(c,true))
                {
                    st = st_attname;
                    attname.resize(1);
                    attname[0] = c;
                }
                break;
            case st_attname:
                if (is_xmlchar(c,false))
                {
                    attname += c;
                }
                else if (is_whitespace(c))
                {
                    attbody = "";
                    attributes[attname] = attbody;
                    attname = "";
                    st = st_ws;
                }
                else if (c == '=')
                {
                    st = st_attbody;
                    attbody.resize(0);
                }
                break;
            case st_attbody:
                if (c == '\"')
                {
                    if (attbody.size() != 0)
                    {
                        unescape(attbody);
                        attributes[attname] = attbody;
                        attname = "";
                        attbody = "";
                        st = st_ws;
                    }
                }
                else
                    attbody += c;
                break;
            }
        }
        if (attname.size() > 0)
            attributes[attname] = "";
    }
    tag_end ++;

    size_t tag_close = tag_end;
    if (selfclosed == false)
    {
        tag_close = xml.find(string("</") + name + ">", tag_end);
        if (tag_close == string::npos)
            return 0;
    }

    string text_raw = xml.substr(tag_end, tag_close - tag_end);
    size_t raw_pos = 0;
    while (raw_pos < text_raw.size())
    {
        MyXmlNode &n = add_child();
        size_t s = n.parse(text_raw, raw_pos);
        if (s <= 0)
            return s;
        raw_pos = s;
    }

    return xml.find_first_of('>', tag_close) + 1;
}

static void do_indent(std::string &xml, int indent)
{
    xml += '\n';
    for (int space_count = 0; space_count < indent; space_count++)
        xml += ' ';
}

bool
MyXmlNode :: generate(std::string &xml, int indent /*=0*/)
{
    bool has_body = true;
    if (name.size() == 0)
    {
        // this is a text-only node
        xml_escape_special(xml, text);
        return true;
    }
    if (validate_xmlness(name) == false)
        return false;
    if (text.size() == 0 && children.size() == 0)
        has_body = false;
    xml += "<" + name;
    map<string,string>::const_iterator it;
    for (it = attributes.begin();
         it != attributes.end();
         it++)
    {
        if (validate_xmlness(it->first) == false)
            return false;
        xml += " ";
        xml += it->first;
        if (it->second.size() > 0)
        {
            xml += "=\"";
            xml_escape_special(xml, it->second);
            xml += "\"";
        }
    }
    if (has_body)
    {
        xml += ">";
        for (size_t ind = 0; ind < children.size(); ind++)
        {
            if (text.size() == 0)
                do_indent(xml, indent+2);
            if (children[ind].generate(xml, indent+2) == false)
                return false;
        }
        xml_escape_special(xml, text);
        if (text.size() == 0)
            do_indent(xml, indent);
        xml += "</" + name + ">";
    }
    else
        xml += "/>";
    return true;
}

#if INCLUDE_MYXML_TEST == 1
int main()
{
    MyXmlNode n;
    string xml("<zero g><one/><five a=\"&amp;b\"/><two c=\"d\" e=\"f\">"
               " &lt;not a tag&gt; <eight> &quot;nine&quot; </eight> "
               "<four/></two>seven</zero>");
    if (n.parse(xml) == 0)
    {
        printf("parse returns error\n");
        return 1;
    }
    printf("original xml:\n%s\n", xml.c_str());
    xml.clear();
    if (n.generate(xml) == false)
        cout << "generate returned false, error in some node?\n";
    else
        printf("generated xml:\n%s\n", xml.c_str());
    return 0;
}
#endif

#if INCLUDE_MYXML_TEST == 2
int
main()
{
    MyXmlNode root;

    root.name = "root";
    root.attributes["date"] = "2019-04-04";
    root.attributes["time"] = "16:23:00.000";
    root.attributes["id"] = "3";
    {
        MyXmlNode &n = root.add_child();
        n.name = "";
        n.text = "\nthis is plain text\n";
    }
    {
        MyXmlNode &n = root.add_child();
        n.name = "node1";
        n.attributes["sample"] = "value1 < value2";
        n.text = "\nthis is node 1\n";

        {
            MyXmlNode &n2 = n.add_child();
            n2.name = "";
            n2.text = "\nthis is MORE plain text\n";
        }
        {
            MyXmlNode &n2 = n.add_child();
            n2.name = "node2";
            n2.attributes["id"] = "4";
            n2.text = "this 'is' \"node 2\" & some chars";
        }
    }
    {
        MyXmlNode &n = root.add_child();
        n.name = "";
        n.text = "\nthis is WAY MORE plain text\n";
    }
    {
        MyXmlNode &n = root.add_child();
        n.name = "node3";
        n.attributes["other_sample"] = "other_value <not a tag>";
        n.attributes["id"] = "5";
    }
    root.text = "\nTEH END\n";
    string xml;
    if (root.generate(xml) == false)
        cout << "generate returned false, error in some node?\n";
    cout << "generated xml:\n" << xml << "\n";
    cout << "generated xml is " << xml.size() << " bytes long\n";
    root.init();
    if (root.parse(xml) == 0)
        printf("parse returns error\n");
    else
    {
        xml.clear();
        root.generate(xml);
        cout << "regenerated xml:\n" << xml << "\n";
    }

    return 0;
}
#endif
