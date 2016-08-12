/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __options_h__
#define __options_h__ 1

namespace pfktop {

    class Options {
        void usage(void);
        bool isOk;
    public:
        Options(int argc, const char * const * argv);
        ~Options(void);
        const bool ok(void) const { return isOk; }
    };

};

#endif /* __options_h__ */
