
#include <stdlib.h>
#include <sstream>
#include "uuz_options.h"
#include "uuz.h"
#include "newbase64.h"
#include "posix_fe.h"

namespace PFK_uuz {

static inline void print_comprencrhmac(const uuzopts &opts)
{
        switch (opts.compression)
        {
        case PFK::uuz::NO_COMPRESSION:
            fprintf(opts.uuz_f, " no-compression");
            break;
        case PFK::uuz::LIBZ_COMPRESSION:
            fprintf(opts.uuz_f, " libz");
            break;
        }
        switch (opts.encryption)
        {
        case PFK::uuz::NO_ENCRYPTION:
            fprintf(opts.uuz_f, " no-encryption");
            break;
        case PFK::uuz::AES256_ENCRYPTION:
            fprintf(opts.uuz_f, " aes256");
            break;
        }
        switch (opts.hmac)
        {
        case PFK::uuz::NO_HMAC:
            fprintf(opts.uuz_f, " no-hmac");
            break;
        case PFK::uuz::HMAC_SHA256HMAC:
            fprintf(opts.uuz_f, " sha256hmac");
            break;
        }
}

bool uuz::encode_m(int Scode)
{
    DEBUGPROTO("encoding msg:\n%s\n", m.DebugString().c_str());

    serialized_pb.resize(0);
    if (m.SerializeToString(&serialized_pb) == false)
    {
        fprintf(stderr, "cannot encode msg of Scode %d\n", Scode);
        return false;
    }
    b64_encoded_pb.resize(0);
    if (Scode == SCODE_VERSION)
        format_hexbytes(b64_encoded_pb, serialized_pb);
    else
        b64.encode(b64_encoded_pb, serialized_pb);

    char * p = (char *) b64_encoded_pb.c_str();
    int remaining = b64_encoded_pb.size();

    while (remaining > MAX_ENCODED_LINE_LENGTH)
    {
        char  tmp = p[MAX_ENCODED_LINE_LENGTH];
        p[MAX_ENCODED_LINE_LENGTH] = 0;
        fprintf(opts.uuz_f, "s%d%s\n", Scode, p);
        opts.check_rollover();
        p[MAX_ENCODED_LINE_LENGTH] = tmp;
        p += MAX_ENCODED_LINE_LENGTH;
        remaining -= MAX_ENCODED_LINE_LENGTH;
    }

    fprintf(opts.uuz_f, "S%d%s\n", Scode, p);
    opts.check_rollover();
    return true;
}

// this is called once at the start.
bool uuz::uuz_encode_emit_s1(void)
{
    if (opts.text_headers)
        fprintf(opts.uuz_f,
                "# program %s protoversion %d using b64 variant %d (%s)\n",
               PFK_UUZ_APP_NAME,
               PFK::uuz::uuz_VERSION_1,
               (int) opts.data_block_variant,
               Base64::variant_name(opts.data_block_variant));
    m.Clear();
    m.set_type(PFK::uuz::uuz_VERSION);
    m.mutable_proto_version()->set_app_name(PFK_UUZ_APP_NAME);
    m.mutable_proto_version()->set_version(PFK::uuz::uuz_VERSION_1);
    m.mutable_proto_version()->set_b64variant(
        (int) opts.data_block_variant);
    if (encode_m(SCODE_VERSION) == false)
        return false;
    m.Clear();
    return true;
}

// this is called once for every file to encode.
int uuz::uuz_encode(void)
{
    int ret = 1;
    PFK::uuz::FileData * fd;
    std::string * fdbuf;
    size_t  got, pos = 0;
    uuzopts::input_file * inf = opts.current_file;

    b64.set_variant(opts.data_block_variant);
    mbedtls_md_starts(&md_ctx);
    if (opts.encryption == PFK::uuz::AES256_ENCRYPTION)
    {
        mbedtls_aes_setkey_enc(&aes_ctx,
                               (const unsigned char *)
                                  opts.encryption_key.c_str(),
                               /*keybits*/256);
    }

    // we don't check for rollover after emitting
    // an S9 record, but we do before emitting an S2,
    // so we will rollover at a file boundary, but not
    // at the last file (don't want a zero length file).
    opts.check_rollover();

    if (opts.text_headers)
    {
        fprintf(opts.uuz_f, "# FILE_INFO '%s' size %" PRIu64,
               inf->path.c_str(),
               (uint64_t) inf->sb.st_size);
        print_comprencrhmac(opts);
        fprintf(opts.uuz_f, "\n");
    }

    m.set_type(PFK::uuz::uuz_FILE_INFO);
    m.mutable_file_info()->set_file_name(inf->path);
    m.mutable_file_info()->set_file_size(inf->sb.st_size);
    // store the mode, but only the rwx bits.
    m.mutable_file_info()->set_file_mode(inf->sb.st_mode & 0777);
    m.mutable_file_info()->set_compression(opts.compression);
    m.mutable_file_info()->set_encryption(opts.encryption);
    m.mutable_file_info()->set_hmac(opts.hmac);
    if (encode_m(SCODE_FILE_INFO) == false)
        goto out;
    m.Clear();

    zs.next_in = NULL;
    zs.avail_in = 0;
    zs.next_out = (Bytef*) obuf;
    zs.avail_out = BUFFER_SIZE;

    m.set_type(PFK::uuz::uuz_FILE_DATA);
    fd = m.mutable_file_data();
    fdbuf = fd->mutable_data();

    if (opts.compression == PFK::uuz::NO_COMPRESSION)
    {
        int cc;
        while (1)
        {
            cc = ::fread(ibuf, 1, BUFFER_SIZE, inf->f);
            if (cc <= 0)
                break;
            fd->set_data_size(cc);
            mbedtls_md_update(&md_ctx, ibuf, cc);
            encrypt(*fdbuf, ibuf, cc);
            fd->set_position(pos);
            if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
            {
                // construct hmac key
                //   encryption key + filename + position
                std::ostringstream hmac_key_str;
                hmac_key_str << opts.encryption_key
                             << inf->path
                             << pos;
                std::string  hmac_key = hmac_key_str.str();

                mbedtls_md_hmac_starts(
                    &hmac_ctx,
                    (const unsigned char*) hmac_key.c_str(),
                    hmac_key.size());

                if (DEBUGFLAG_HMAC)
                {
                    std::string key_string;
                    format_hexbytes(key_string, hmac_key);
                    fprintf(stderr, "HMAC STARTS with key %s\n",
                            key_string.c_str());
                }

                mbedtls_md_hmac_update(&hmac_ctx,
                                       (const unsigned char *)fdbuf->c_str(),
                                       fdbuf->size());
                DEBUGHMAC("HMAC update with %" PRIu32
                          " bytes %08x %08x %08x %08x\n",
                          (uint32_t) fdbuf->size(),
                          ((uint32_t*)fdbuf->c_str())[0],
                          ((uint32_t*)fdbuf->c_str())[1],
                          ((uint32_t*)fdbuf->c_str())[2],
                          ((uint32_t*)fdbuf->c_str())[3]);

                std::string *hmac = fd->mutable_hmac();
                hmac->resize(md_size);
                mbedtls_md_hmac_finish(&hmac_ctx,
                                       (unsigned char*) hmac->c_str());
                DEBUGHMAC("HMAC calculated %08x %08x %08x %08x\n",
                          ((uint32_t*)hmac->c_str())[0],
                          ((uint32_t*)hmac->c_str())[1],
                          ((uint32_t*)hmac->c_str())[2],
                          ((uint32_t*)hmac->c_str())[3]);
            }
            if (opts.text_headers)
            {
                fprintf(opts.uuz_f,
                        "# FILE DATA %" PRIu32 " bytes at position %" PRIu64,
                        (uint32_t) fdbuf->size(), (uint64_t) pos);
                print_comprencrhmac(opts);
                fprintf(opts.uuz_f, "\n");
            }
            encode_m(SCODE_DATA);
            pos += cc;
        }
    }
    else if (opts.compression == PFK::uuz::LIBZ_COMPRESSION)
    {
        bool reader_done = false;
        while (1)
        {
            int cc;
            if (!reader_done && zs.avail_in == 0)
            {
                cc = ::fread(ibuf, 1, BUFFER_SIZE, inf->f);
                if (cc <= 0)
                    reader_done = true;
                else
                {
                    mbedtls_md_update(&md_ctx, ibuf, cc);
                    zs.next_in = (Bytef*) ibuf;
                    zs.avail_in = cc;
                }
            }

            DEBUGLIBZ("before deflate: avail_in %d avail_out %d\n",
                  zs.avail_in, zs.avail_out);

            int flush = reader_done ? Z_FINISH : Z_NO_FLUSH;
            DEBUGLIBZ("calling deflate with flush %d (%s)\n", flush,
                  flushstring(flush));
            cc = deflate(&zs, flush);

            DEBUGLIBZ("deflate returns %d (%s), avail_in %d avail_out %d\n",
                  cc, deflateretstring(cc), zs.avail_in, zs.avail_out);

            if (zs.avail_out < BUFFER_SIZE)
            {
                int remaining = BUFFER_SIZE - zs.avail_out;

                if (opts.text_headers)
                {
                    fprintf(opts.uuz_f,
                            "# FILE DATA %d bytes at position %" PRIu64,
                           remaining, (uint64_t) pos);
                    print_comprencrhmac(opts);
                    fprintf(opts.uuz_f, "\n");
                }
                encrypt(*fdbuf, obuf, remaining);

                if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
                {
                    // construct hmac key
                    //   encryption key + filename + position
                    std::ostringstream hmac_key_str;
                    hmac_key_str << opts.encryption_key
                                 << inf->path
                                 << pos;
                    std::string  hmac_key = hmac_key_str.str();

                    mbedtls_md_hmac_starts(
                        &hmac_ctx,
                        (const unsigned char*) hmac_key.c_str(),
                        hmac_key.size());

                    if (DEBUGFLAG_HMAC)
                    {
                        std::string key_string;
                        format_hexbytes(key_string, hmac_key);
                        fprintf(stderr, "HMAC STARTS with key %s\n",
                                key_string.c_str());
                    }

                    mbedtls_md_hmac_update(&hmac_ctx,
                                           (unsigned char *) fdbuf->c_str(),
                                           fdbuf->size());
                    DEBUGHMAC("HMAC update with %" PRIu32 " bytes "
                              "%08x %08x %08x %08x\n",
                              (uint32_t) fdbuf->size(),
                              ((uint32_t*)fdbuf->c_str())[0],
                              ((uint32_t*)fdbuf->c_str())[1],
                              ((uint32_t*)fdbuf->c_str())[2],
                              ((uint32_t*)fdbuf->c_str())[3]);

                    std::string *hmac = fd->mutable_hmac();
                    hmac->resize(md_size);
                    mbedtls_md_hmac_finish(&hmac_ctx,
                                           (unsigned char*) hmac->c_str());
                    DEBUGHMAC("HMAC calculated %08x %08x %08x %08x\n",
                              ((uint32_t*)hmac->c_str())[0],
                              ((uint32_t*)hmac->c_str())[1],
                              ((uint32_t*)hmac->c_str())[2],
                              ((uint32_t*)hmac->c_str())[3]);
                    mbedtls_md_hmac_reset(&hmac_ctx);
                }

                fd->set_data_size(remaining);
                fd->set_position(pos);
                pos += remaining;
                encode_m(SCODE_DATA);
                fdbuf->resize(0);

                zs.next_out = (Bytef*) obuf;
                zs.avail_out = BUFFER_SIZE;
            }

            if (cc == Z_STREAM_END)
                break;

            if (cc < 0 && cc != Z_BUF_ERROR)
            {
                fprintf(stderr, "ZERROR %d (%s)\n", cc, deflateretstring(cc));
                exit(1);
            }
        }
    }
    m.Clear();

    m.set_type(PFK::uuz::uuz_FILE_COMPLETE);
    m.mutable_file_complete()->set_compressed_size(pos);

    {
        std::string *sha = m.mutable_file_complete()->mutable_sha();
        sha->resize(md_size);
        mbedtls_md_finish(&md_ctx, (unsigned char*) sha->c_str());
    }

    if (opts.text_headers)
        fprintf(opts.uuz_f, "# FILE COMPLETE, compressed size %" PRIu64 "\n",
               (uint64_t) pos);
    if (encode_m(SCODE_COMPLETE) == false)
        goto out;

    ret = 0;
    fprintf(stderr, "encode '%s' successful\n", inf->path.c_str());

out:
    return ret;
}

void uuz :: encrypt(std::string &out,
                    unsigned char *in, int ilen)
{
    switch (opts.encryption)
    {
    case PFK::uuz::NO_ENCRYPTION:
        out.assign((char*)in, ilen);
        break;

    case PFK::uuz::AES256_ENCRYPTION:
    {
        // need to pad up to 16 to use AES256.
        int sz = ilen;
        int blocks = (sz + 15) / 16;
        int newsize = blocks * 16;
        int pad = newsize - sz;

        DEBUGMBED("size %d blocks %d newsize %d pad %d\n",
                  sz, blocks, newsize, pad);

        if (newsize != sz)
        {
            // fill the pad with random fill.
            for (int ind = 0; ind < pad; ind++)
                in[sz + ind] = (char)(random() & 0xFF);
        }

        out.resize(newsize);
        unsigned char * outp = (unsigned char *) out.c_str();
        unsigned char * inp = (unsigned char *) in;

        for (int ind = 0; ind < blocks; ind++)
        {
            mbedtls_aes_crypt_ecb(
                &aes_ctx, MBEDTLS_AES_ENCRYPT,
                inp, outp);

            inp += 16;
            outp += 16;
        }
        break;
    }
    }
}

}; // namespace PFK_uuz
