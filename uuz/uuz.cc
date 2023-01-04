
#include "posix_fe.h"
#include "uuz_options.h"
#include "uuz.h"

namespace PFK_uuz {

static void * zalloc( void * opaque, uInt items, uInt size)
{
//    uuz * puuz = (uuz*) opaque;
    return calloc(items, size);
}

static void zfree( void * opaque, void * ptr )
{
//    uuz * puuz = (uuz*) opaque;
    free(ptr);
}

uuz :: uuz(int argc, char ** argv)
    : opts(argc, argv)
{
    if (!opts)
        opts.usage();
    serialized_pb.reserve(SERIALIZED_BUFSIZE);
    b64_encoded_pb.reserve(B64_ENCODED_PBSIZE);
    partial_serialized_pb.reserve(SERIALIZED_BUFSIZE);
    got_version = false;
    output_f = NULL;
    output_filesize = -1;
    expected_pos = -1;
    zs.zalloc = &zalloc;
    zs.zfree = &zfree;
    zs.opaque = this;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_md_init(&md_ctx);
    const mbedtls_md_info_t * md_info =
        mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (md_info)
    {
        mbedtls_md_setup(&hmac_ctx, md_info, /*hmac*/1);
        mbedtls_md_setup(&md_ctx, md_info, /*hmac*/0);
        md_size = (int) mbedtls_md_get_size(md_info);
    }
    else
    {
        fprintf(stderr, "ERROR: MBEDTLS_MD_SHA256 NOT FOUND\n");
        exit(1); // bad
    }
}

uuz :: ~uuz(void)
{
    if (output_f)
    {
        fclose(output_f);
        unlink(output_filename.c_str());
        output_f = NULL;
    }
    mbedtls_aes_free(&aes_ctx);
    mbedtls_md_free(&md_ctx);
    mbedtls_md_free(&hmac_ctx);
}

static int decimal_digits(uint32_t  v)
{
    int ret = 0;
    do {
        ret++;
        v /= 10;
    } while (v > 0);
    return ret;
}

int uuz :: main(void)
{
    int ret = 1;

    switch (opts.mode)
    {
    case uuzopts::ENCODE:
        if (!uuz_encode_emit_s1())
            break;
        for (auto inf : opts.input_files)
        {
            opts.current_file = inf;
            deflateInit(&zs, 9);
            ret = uuz_encode();
            deflateEnd(&zs);
            if (ret != 0)
                break;
        }
        break;

    case uuzopts::DECODE:
    case uuzopts::LIST:
    {
        uuz_decode_ret_t  dr;
        do {
            inflateInit(&zs);
            dr = uuz_decode(/*list_only*/opts.mode == uuzopts::LIST);
            inflateEnd(&zs);
            if (dr == DECODE_COMPLETE)
                ret = 0;
        } while (dr == DECODE_MORE);
        break;
    }
    }

    if (opts.mode == uuzopts::LIST)
    {
        int sizewidth = 4; // width of "size"
        int csizewidth = 5; // width of "csize"
        int filenamewidth = 8; // width of "filename"

        for (auto &li : list_output)
        {
            int w = decimal_digits(li->size);
            if (w > sizewidth) sizewidth = w;
            w = decimal_digits(li->csize);
            if (w > csizewidth) csizewidth = w;
            w = (int) li->filename.size();
            if (w > filenamewidth) filenamewidth = w;
        }

        printf("%-11s %*s %*s  %% hmac sha  filename\n",
               "---mode---", sizewidth, "size", csizewidth, "csize");

        for (auto &li : list_output)
        {
            // first do mode.
            putchar('-'); // always file, never dir or special
            for (int ugo = 2; ugo >= 0; ugo--)
            {
                uint32_t m = li->mode >> (ugo*3);
                if (m & 4) putchar('r'); else putchar('-');
                if (m & 2) putchar('w'); else putchar('-');
                if (m & 1) putchar('x'); else putchar('-');
            }
            putchar(' ');

            if (li->csize != 0)
            {
                printf(" %*" PRIu64 " %*" PRIu64 " %2d %s %s %s\n",
                       sizewidth, li->size,
                       csizewidth, li->csize,
                       li->percent,
                       li->hmac_status.c_str(),
                       li->sha_status.c_str(),
                       li->filename.c_str());
            }
            else
            {
                printf(" %*" PRIu64 " %*s  - %s %s %s\n",
                       sizewidth, li->size,
                       csizewidth, "-",
                       li->hmac_status.c_str(),
                       li->sha_status.c_str(),
                       li->filename.c_str());
            }
        }
    }

    return ret;
}

void uuz :: format_hexbytes(std::string &out, const std::string &in)
{
    out.resize(in.size() * 2);
    char * p = (char*) out.c_str();
    for (int ind = 0; ind < in.size(); ind++)
    {
        unsigned char c = (unsigned char) in[ind];
        sprintf(p, "%02x", c);
        p += 2;
    }
}

}; // namespace PFK_uuz

extern "C"
int uuz_main(int argc, char ** argv)
{
    PFK_uuz::uuz  uuz(argc, argv);

    if (!uuz)
        return 1;

    return uuz.main();
}
