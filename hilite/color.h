/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include <string>

class Hilite_color {
    int color;
    int mods; // bitmap
public:
    Hilite_color(void);
    ~Hilite_color(void);
    bool set_color(const std::string &color);
    bool set_modifier(const std::string &mod);
    void finalize(void);
    void print(void);
    std::string color_string;
    std::string normal_color_string;
};

