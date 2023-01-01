
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

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


int uuz :: main(void)
{
    int ret = 1;

    switch (opts.mode)
    {
    case PFK_uuz::uuzopts::ENCODE:
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

    case PFK_uuz::uuzopts::DECODE:
    {
        uuz_decode_ret_t  dr;
        do {
            inflateInit(&zs);
            dr = uuz_decode();
            inflateEnd(&zs);
            if (dr == DECODE_COMPLETE)
                ret = 0;
        } while (dr == DECODE_MORE);
        break;
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
