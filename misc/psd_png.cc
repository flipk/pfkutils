
#include "psd_png.h"

PngHeatMap :: PngHeatMap(const char *fname, int width, int height)
{
    _ok = false;

    fp = fopen(fname, "wb");
    if (fp == NULL)
        return;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                      /*error ptr*/ NULL,
                                      /*error func*/ NULL,
                                      /*warning func*/ NULL);

    if (png_ptr == NULL)
    {
        fclose(fp);
        return;
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL)
    {
        fclose(fp);
        png_destroy_write_struct(&png_ptr,  NULL);
        return;
    }

    if (setjmp(png_jmpbuf(png_ptr)))
    {
        /* If we get here, we had a problem writing the file */
        fclose(fp);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        return;
    }

    png_init_io(png_ptr, fp);

    png_set_IHDR(png_ptr, info_ptr,
                 width, height, bit_depth,
                 PNG_COLOR_TYPE_PALETTE,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE,
                 PNG_FILTER_TYPE_BASE);

    init_palette();

    png_set_PLTE(png_ptr, info_ptr, palette, PALETTE_SIZE);

#if 0
    /* Optional significant bit (sBIT) chunk */
    png_color_8 sig_bit;
    /* If we are dealing with a grayscale image then */
    sig_bit.gray = true_bit_depth;
    /* Otherwise, if we are dealing with a color image then */
    sig_bit.red = true_red_bit_depth;
    sig_bit.green = true_green_bit_depth;
    sig_bit.blue = true_blue_bit_depth;
    /* If the image has an alpha channel then */
    sig_bit.alpha = true_alpha_bit_depth;
    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    /* Optional gamma chunk is strongly suggested if you have any guess
     * as to the correct gamma of the image.
     */
    png_set_gAMA(png_ptr, info_ptr, gamma);

    /* Optionally write comments into the image */
    {
        png_text text_ptr[3];

        char key0[]="Title";
        char text0[]="Mona Lisa";
        text_ptr[0].key = key0;
        text_ptr[0].text = text0;
        text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
        text_ptr[0].itxt_length = 0;
        text_ptr[0].lang = NULL;
        text_ptr[0].lang_key = NULL;

        char key1[]="Author";
        char text1[]="Leonardo DaVinci";
        text_ptr[1].key = key1;
        text_ptr[1].text = text1;
        text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
        text_ptr[1].itxt_length = 0;
        text_ptr[1].lang = NULL;
        text_ptr[1].lang_key = NULL;

        char key2[]="Description";
        char text2[]="<long text>";
        text_ptr[2].key = key2;
        text_ptr[2].text = text2;
        text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
        text_ptr[2].itxt_length = 0;
        text_ptr[2].lang = NULL;
        text_ptr[2].lang_key = NULL;

        png_set_text(write_ptr, write_info_ptr, text_ptr, 3);
    }
#endif

    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);
    png_set_swap(png_ptr);

    _ok = true;
}

void PngHeatMap :: init_palette(void)
{
    int ind;
#if 0
    // this mimics the "jet" colormap in matlab.
    for (ind = 0; ind < 32; ind++)
    {
        palette[ind].red = 0;
        palette[ind].green = 0;
        palette[ind].blue = 128 + (ind * 4);
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+32].red = 0;
        palette[ind+32].green = ind * 4;
        palette[ind+32].blue = 255;
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+96].red = ind * 4;
        palette[ind+96].green = 255;
        palette[ind+96].blue = 255 - (ind * 4);
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+160].red = 255;
        palette[ind+160].green = 255 - (ind * 4);
        palette[ind+160].blue = 0;
    }
    for (ind = 0; ind < 32; ind++)
    {
        palette[ind+224].red = 255 - (ind * 4);
        palette[ind+224].green = 0;
        palette[ind+224].blue = 0;
    }
#endif
#if 1
    // modified jet, black at the bottom and bright red at top.
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind].red = 0;
        palette[ind].green = 0;
        palette[ind].blue = (ind * 4);
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+64].red = 0;
        palette[ind+64].green = ind * 4;
        palette[ind+64].blue = 255;
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+128].red = ind * 4;
        palette[ind+128].green = 255;
        palette[ind+128].blue = 255 - (ind * 4);
    }
    for (ind = 0; ind < 64; ind++)
    {
        palette[ind+192].red = 255;
        palette[ind+192].green = 255 - (ind * 4);
        palette[ind+192].blue = 0;
    }

#endif
#if 0
    for (ind = 0; ind < 256; ind++)
    {
        printf("color %2d : %.4f %.4f %.4f\n", ind,
               (float) palette[ind].red / 256.0,
               (float) palette[ind].green / 256.0,
               (float) palette[ind].blue / 256.0);
    }
#endif
}

bool PngHeatMap :: write_line(const unsigned char *bytes)
{
    if (setjmp(png_jmpbuf(png_ptr)))
        return false;

    png_write_row(png_ptr, (png_const_bytep) bytes);
    return true;
}

PngHeatMap :: ~PngHeatMap(void)
{
    if (_ok)
    {
        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
    }
}

#ifdef __INCLUDE_MAIN__

int main(int argc, char ** argv)
{
    int width = 256;
    int height = 64;
    unsigned char image[height][width];

    {
        unsigned char ctr = 0;
        for (int r = 0; r < height; r++)
            for (int c = 0; c < width; c++)
                image[r][c] = ctr++;
    }

    {
        PngHeatMap  hm("obj/test_gradient.png", width, height);

        if (!hm.ok())
            return 1;

        for (int row = 0; row < height; row++)
            hm.write_line((const unsigned char *)&image[row]);
    }

    return 0;
}

#endif // __INCLUDE_MAIN__
