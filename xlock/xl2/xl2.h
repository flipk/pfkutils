/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#ifndef __XL2_H__
#define __XL2_H__

#include <X11/Xlib.h>
#include <string>

#define DEFAULT_FONT "pfk20"
#define FALLBACK_FONT "fixed"
#define DEF_BG "White"
#define DEF_FG "Black"
#define PROGRAM_NAME "XLOCK2"

struct xl2_screen {
    static const int NUMCOLORS = 64;
    int screen; // index in the screens array
    Display * dsp; // for convenience
    Window root;
    Window win;
    Window icon;
    Screen * scr;
    long bgcol;
    long fgcol;
    GC   gc;
    GC   textgc;
    int  npixels;
    u_long pixels[NUMCOLORS];
    int iconx;
    int icony;
};

class LockProcFactory;

class LockProc {
    friend class LockProcFactory;
    static int factory_count;
    static const int max_factories = 20;
    static LockProcFactory * factories[max_factories];
    static void register_lp(LockProcFactory * lpf);
protected:
    xl2_screen &s;
public:
    LockProc(xl2_screen &_s) : s(_s) { }
    virtual ~LockProc(void) { };
    virtual void draw(void) = 0;
    static LockProc * make(const std::string &name, xl2_screen &s);
    static LockProc * make_random(xl2_screen &s);
};

class LockProcFactory
{
public:
    LockProcFactory * next;
    const char * name;
    LockProcFactory(const char * _name)
    {
        name = _name;
        LockProc::register_lp(this);
    }
    virtual LockProc * make(xl2_screen &_s) = 0;
};

extern int get_idle_seconds(Display *d);

#endif /* __XL2_H__ */
