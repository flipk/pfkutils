
#ifndef __MYXML_H__
#define __MYXML_H__ 1

#include <string>
#include <vector>
#include <map>

struct MyXmlNode
{
    // a 0-length name string means a text value
    // in between other nodes.
    std::string                       name;
    std::map<std::string,std::string> attributes;
    std::vector<MyXmlNode>            children;
    std::string                       text;
    MyXmlNode &add_child(void) {
        size_t ind = children.size();
        children.resize(ind+1);
        return children[ind];
    }
    void init(void) {
        name.clear();
        attributes.clear();
        children.clear();
        text.clear();
    }
    // return position after parsing, or 0 for error.
    size_t parse(const std::string &xml, size_t start_pos = 0);
    // return false for failure
    bool generate(std::string &xml, int indent=0);
};

#endif /* __MYXML_H__ */
