/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __PSD_PNG_H__
#define __PSD_PNG_H__ 1

#include <png.h>
#include <string.h>

class PngHeatMap
{
    static const int PALETTE_SIZE = 256;
    const int bit_depth = 8;
    const int bytes_per_pixel = 1;

    bool _ok;
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
    png_color palette[PALETTE_SIZE];

    void init_palette(void);
public:
    PngHeatMap(const char *fname, int width, int height);
    ~PngHeatMap(void);
    bool ok(void) const { return _ok; }
    bool write_line(const unsigned char *bytes);
};

#endif // __PSD_PNG_H__
