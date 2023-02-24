
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
    DEBUGPROTO("encoding msg of size %u:\n%s\n",
               (uint32_t) m. PROTOBUF_BYTE_SIZE(),
               m.DebugString().c_str());

    // base64 encodes 3 binary bytes to 4. for most b64 variants,
    // you don't have to worry about that multiple of 3 -- if it's
    // shorter, the encoder pads and the decoder truncates to the correct
    // length. but UUENCODE variant is broken in this manner, it cannot
    // pad or detect properly, so you can get 1 or 2 extra bytes of junk
    // (rounding up to the next multiple of 3). this can screw up the
    // protobuf decode, so we encode the length of the protobuf in
    // the stream.

    google::protobuf::Message * msg_to_serialize = &m;
    serialized_pb.resize(0);

    if (Scode != SCODE_VERSION)
    {
        if (encryption == PFK::uuz::AES256_ENCRYPTION)
        {
            if (m.SerializeToString(&serialized_pb) == false)
            {
                fprintf(stderr, "cannot encode msg of Scode %d\n", Scode);
                exit(1);
            }
            encrypted_container.Clear();
            int message_size = serialized_pb.size();


            if (Scode == SCODE_FILE_INFO)
            {
                // start a new IV for each file.
                iv.resize(mbed_iv_length);
                fillRandomBuffer((void*) iv.c_str(), mbed_iv_length);
                encrypted_container.set_iv(iv);
            }

            // leave enough room for encrypt to PAD if it needs to.
            // ... this is gross, because encrypt() will write to its
            // input parameter *beyond* the size you specify.
            serialized_pb.resize( message_size + mbed_aes_block_size );

            // but pass the original message size as the length.
            encrypt(*encrypted_container.mutable_data(),
                    (unsigned char *) serialized_pb.c_str(),
                    message_size);

            // be sure original message size makes it to decoder.
            encrypted_container.set_data_size(message_size);

            uint32_t salt;
            fillRandomBuffer(&salt, 4);
            encrypted_container.set_salt(salt);

            if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
            {
                // construct hmac key
                //   encryption key + data_size + salt
                std::ostringstream hmac_key_str;
                hmac_key_str << opts.encryption_key
                             << message_size
                             << salt;
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
                                       (unsigned char *) serialized_pb.c_str(),
                                       message_size);
                DEBUGHMAC("HMAC update with %" PRIu32 " bytes "
                          "%08x %08x %08x %08x\n",
                          (uint32_t) message_size,
                          ((uint32_t*)serialized_pb.c_str())[0],
                          ((uint32_t*)serialized_pb.c_str())[1],
                          ((uint32_t*)serialized_pb.c_str())[2],
                          ((uint32_t*)serialized_pb.c_str())[3]);

                std::string *hmac = encrypted_container.mutable_hmac();
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

            // overwrite serialized_pb with encrypted message.
            encrypted_container.SerializeToString(&serialized_pb);

            DEBUGPROTO("encoding encrypted msg of size %u:\n%s\n",
                       (uint32_t) encrypted_container.PROTOBUF_BYTE_SIZE(),
                       encrypted_container.DebugString().c_str());

            msg_to_serialize = &encrypted_container;
        }
    }

    serialized_pb.resize(0);
    {
        google::protobuf::io::StringOutputStream zos(&serialized_pb);
        {
            google::protobuf::io::CodedOutputStream cos(&zos);
            uint32_t  len = (uint32_t) msg_to_serialize->PROTOBUF_BYTE_SIZE();
            cos.WriteVarint32( len );
            if (msg_to_serialize->SerializeToCodedStream(&cos) == false)
            {
                fprintf(stderr, "cannot encode msg of Scode %d\n", Scode);
                exit(1);
            }
        } // cos destroyed here
    } // zos destroyed here.

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
    compression = opts.compression;
    encryption = opts.encryption;
    hmac = opts.hmac;

    if (opts.text_headers)
    {
        compression_name = "no compression";
        switch (opts.compression)
        {
        case PFK::uuz::NO_COMPRESSION:
            // already set
            break;
        case PFK::uuz::LIBZ_COMPRESSION:
            compression_name = "libz";
            break;
        }

        encryption_name = "no encryption";
        switch (encryption)
        {
        case PFK::uuz::NO_ENCRYPTION:
            // already set
            break;
        case PFK::uuz::AES256_ENCRYPTION:
            encryption_name = "AES256";
            break;
        }

        hmac_name = "no hmac";
        switch (hmac)
        {
        case PFK::uuz::NO_HMAC:
            // already set
            break;
        case PFK::uuz::HMAC_SHA256HMAC:
            hmac_name = "SHA256HMAC";
            break;
        }

        fprintf(opts.uuz_f,
                "# program %s protoversion %d b64variant %d (%s)"
                " %s %s %s\n",
               PFK_UUZ_APP_NAME,
               PFK::uuz::uuz_VERSION_2,
               (int) opts.data_block_variant,
               Base64::variant_name(opts.data_block_variant),
               compression_name,
               encryption_name,
               hmac_name);
    }
    m.Clear();
    m.set_type(PFK::uuz::uuz_VERSION);
    m.mutable_proto_version()->set_app_name(PFK_UUZ_APP_NAME);
    m.mutable_proto_version()->set_version(PFK::uuz::uuz_VERSION_2);
    m.mutable_proto_version()->set_b64variant(
        (int) opts.data_block_variant);
    m.mutable_proto_version()->set_compression(opts.compression);
    m.mutable_proto_version()->set_encryption(opts.encryption);
    m.mutable_proto_version()->set_hmac(opts.hmac);
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
            fdbuf->resize(BUFFER_SIZE);
            cc = ::fread((void*) fdbuf->c_str(), 1, BUFFER_SIZE, inf->f);
            if (cc <= 0)
                break;
            mbedtls_md_update(&md_ctx,
                              (const unsigned char *) fdbuf->c_str(),
                              cc);
            fd->set_data_size(cc);
            fdbuf->resize(cc);
            fd->set_position(pos);
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
                fdbuf->assign((char*) obuf, remaining);
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
        // need to pad up to mbed_aes_block_size to use AES256.
        int sz = ilen;
        int blocks = (sz + (mbed_aes_block_size-1)) / mbed_aes_block_size;
        int newsize = blocks * mbed_aes_block_size;
        int pad = newsize - sz;

        DEBUGMBED("size %d blocks %d newsize %d pad %d\n",
                  sz, blocks, newsize, pad);

        if (newsize != sz)
            fillRandomBuffer((void*)(in + sz), pad);

        out.resize(newsize);

        if (DEBUGFLAG_MBED)
        {
            std::string  hex_iv;
            std::string  hex_in;
            format_hexbytes(hex_iv, iv);
            format_hexbytes(hex_in, std::string((char*)in, ilen));
            fprintf(stderr, "AES encrypt %u bytes, iv before %s\n"
                    "inbytes %s\n",
                    mbed_aes_block_size * blocks,
                    hex_iv.c_str(),
                    hex_in.c_str());
        }

        mbedtls_aes_crypt_cbc(
            &aes_ctx, MBEDTLS_AES_ENCRYPT,
            mbed_aes_block_size * blocks,
            (unsigned char *) iv.c_str(),
            (unsigned char *) in,
            (unsigned char *) out.c_str());

        if (DEBUGFLAG_MBED)
        {
            std::string  hex_iv;
            std::string  hex_out;
            format_hexbytes(hex_iv, iv);
            format_hexbytes(hex_out, out);
            fprintf(stderr, "AES encrypt iv after %s\n"
                    "outbytes %s\n",
                    hex_iv.c_str(),
                    hex_out.c_str());
        }

        break;
    }
    }
}

}; // namespace PFK_uuz
