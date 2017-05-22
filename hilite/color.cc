
#include "color.h"
#include <stdio.h>
#include <sstream>

using namespace std;

static const char * valid_colors[] = {
    "black",
    "red",
    "green",
    "yellow",
    "blue",
    "purple",
    "cyan",
    "white",
    "normal",
    NULL
};
#define NORMAL_COLOR_IND 8

static const int normal_color_codes[8] = {
    30, 31, 32, 33, 34, 35, 36, 37
};
static const int bright_color_codes[8] = {
    30, 91, 92, 93, 94, 95, 96, 97
};
static const int bold_code_on = 1;
static const int bold_code_off = 21;
static const int inverse_code_on = 7;
static const int inverse_code_off = 27;
static const int normal_code = 0;

static const char * valid_mods[] = {
    "bold",
    "inverse",
    "bright",
    NULL
};
static const int mod_bold = 1;
static const int mod_inverse = 2;
static const int mod_bright = 4;

Hilite_color :: Hilite_color(void)
{
    color = -1;
    mods = 0;
}

Hilite_color :: ~Hilite_color(void)
{
}

bool
Hilite_color :: set_color(const string &colorstr)
{
    int ind;
    for (ind = 0; valid_colors[ind] != NULL; ind++)
        if (colorstr == valid_colors[ind])
            break;
    if (valid_colors[ind] == NULL)
    {
        printf("color '%s' is not a valid ANSI color\n",
               colorstr.c_str());
        return false;
    }
    color = ind;
    return true;
}

bool
Hilite_color :: set_modifier(const string &mod)
{
    int ind;
    for (ind = 0; valid_mods[ind] != NULL; ind++)
        if (mod == valid_mods[ind])
            break;
    if (valid_mods[ind] == NULL)
    {
        printf("mod '%s' is not a valid modifier\n", mod.c_str());
        return false;
    }
    mods |= (1 << ind);
    return true;
}

void
Hilite_color :: print(void)
{
    printf("%s", valid_colors[color]);
    for (int ind = 0; valid_mods[ind] != NULL; ind++)
        if (mods & (1 << ind))
            printf(" %s",valid_mods[ind]);
}

void
Hilite_color :: finalize(void)
{
    ostringstream str;
    ostringstream str2;
    str  << (char) 27 << '[';
    str2 << (char) 27 << '[';
    if (color != NORMAL_COLOR_IND)
    {
        if (mods & mod_bright)
            str << bright_color_codes[color];
        else
            str << normal_color_codes[color];
        if (mods & mod_bold)
            str << ";" << bold_code_on;
        if (mods & mod_inverse)
            str << ";" << inverse_code_on;

        str2 << normal_code;
    }
    else
    {
        bool first = true;
        if (mods & mod_bold)
        {
            str << bold_code_on;
            str2 << bold_code_off;
            first = false;
        }
        if (mods & mod_inverse)
        {
            if (!first)
            {
                str << ";";
                str2 << ";";
            }
            str << inverse_code_on;
            str2 << inverse_code_off;
// not needed in this case:  first = false;
        }
    }
    str << 'm';
    str2 << 'm';
    color_string = str.str();
    normal_color_string = str2.str();
}
