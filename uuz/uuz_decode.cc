
#include "posix_fe.h"
#include "uuz_options.h"
#include "uuz.h"
#include UUZ_PROTO_HDR

namespace PFK_uuz {

bool uuz :: getline(void)
{
    b64_encoded_pb.resize(0);
    while (1)
    {
        int c = fgetc(opts.current_file->f);
        if (c == EOF)
            return false;
        if (c == '\r' || c == '\n')
            // b64_encoded_pb is complete?
            break;
        b64_encoded_pb += (char) c;
    }
    return true;
}

uuz :: uuz_decode_ret_t
uuz :: uuz_decode(void)
{
    uuz_decode_ret_t ret = DECODE_ERR;

    serialized_pb.resize(0);

    mbedtls_md_starts(&md_ctx);
    if (opts.encryption == PFK::uuz::AES256_ENCRYPTION)
    {
        mbedtls_aes_setkey_dec(&aes_ctx,
                               (const unsigned char *)
                                  opts.encryption_key.c_str(),
                               /*keybits*/256);
    }

    bool done = false;
    while (!done)
    {
        if (!getline())
        {
            ret = DECODE_COMPLETE;
            done = true;
            break;
        }
        if (b64_encoded_pb.size() <= 2)
            continue;
        char s = b64_encoded_pb[0];
        if (s == '#')
        {
            // ignore it quietly.
            continue;
        }
        if (s != 'S'  &&  s != 's')
        {
            DEBUGDECODE("line does not start with [Ss]: %s\n",
                  b64_encoded_pb.c_str());
            continue;
        }
        char type = b64_encoded_pb[1];
        if (!isdigit(type))
        {
            DEBUGDECODE("line does not have a digit: %s\n",
                  b64_encoded_pb.c_str());
            continue;
        }
        int Scode = 0;
        switch (type)
        {
        case '1':   Scode = SCODE_VERSION;   break;
        case '2':   Scode = SCODE_FILE_INFO; break;
        case '3':   Scode = SCODE_DATA;      break;
        case '9':   Scode = SCODE_COMPLETE;  break;
        default:
            DEBUGDECODE("not a valid Srecord type: %s\n",
                  b64_encoded_pb.c_str());
        }
        if (Scode == 0)
            continue;
        b64_encoded_pb.erase(0,2);
        DEBUGDECODE("found an %c%c record: %s\n",
              s, type, b64_encoded_pb.c_str());
        switch (decode_b64(s, Scode))
        {
        case INVALID_B64:
            DEBUGDECODE("b64 decode failure on Scode %d\n", Scode);
            serialized_pb.resize(0);
            break;

        case PARTIAL_B64:
            DEBUGDECODE("continuing piece of Scode %d\n", Scode);
            break;

        case COMPLETE_B64:
            DEBUGDECODE("completed Scode %d\n", Scode);
            if (decode_m())
                switch (Scode)
                {
                case SCODE_VERSION:
                    handle_s1_version();
                    break;

                case SCODE_FILE_INFO:
                    if (handle_s2_file_info() == false)
                        done = true;
                    break;

                case SCODE_DATA:
                    handle_s3_data();
                    break;

                case SCODE_COMPLETE:
                    handle_s9_complete();
                    done = true;
                    ret = DECODE_MORE;
                    break;
                }
            else
            {
                fprintf(stderr, "ERROR: failed to proto decode "
                        "Scode %d\n", Scode);
            }
            serialized_pb.resize(0);
            break;
        }
    }

    return ret;
}

bool uuz :: decode_version(std::string &out, const std::string &in)
{
    char byte[3];
    if (in.size() % 1)
    {
        fprintf(stderr, "input version hex size is ODD\n");
        return false;
    }
    out.resize(in.size() / 2);
    unsigned char * outp = (unsigned char *) out.c_str();
    char * inp = (char *) in.c_str();
    byte[2] = 0;
    for (int ind = 0; ind < in.size(); ind += 2)
    {
        byte[0] = inp[0];
        byte[1] = inp[1];
        uint32_t  v;
        if (pxfe_utils::parse_number(byte, &v, 16) == false)
            return false;
        *outp = (unsigned char) v;
        inp += 2;
        outp ++;
    }
    return true;
}

uuz :: decode_b64_res_t uuz :: decode_b64(char s, int Scode)
{
    partial_serialized_pb.resize(0);
    if (Scode == SCODE_VERSION)
    {
        if (decode_version(partial_serialized_pb, b64_encoded_pb) == false)
            return INVALID_B64;
    }
    else
    {
        if (b64.decode(partial_serialized_pb, b64_encoded_pb) == false)
            return INVALID_B64;
    }

    serialized_pb.append(partial_serialized_pb);
    partial_serialized_pb.resize(0);

    return (s == 'S') ? COMPLETE_B64 : PARTIAL_B64;
}

bool uuz:: decode_m(void)
{
    google::protobuf::io::ArrayInputStream ais(
        serialized_pb.data(), serialized_pb.size());

    {
        google::protobuf::io::CodedInputStream cis(&ais);
        uint32_t len = 0;
        if (cis.ReadVarint32(&len) == false)
            return false;
        auto old = cis.PushLimit(len);
        m.Clear();
        int r = m.ParseFromCodedStream(&cis);
        cis.PopLimit(old);
        if (r)
        {
            DEBUGPROTO("parsed proto:\n%s\n", m.DebugString().c_str());
            return true;
        }
    }
    return false;
}

void uuz :: handle_s1_version(void)
{
    if (m.type() != PFK::uuz::uuz_VERSION)
    {
        fprintf(stderr, "S1 proto type mismatch!\n");
        return;
    }
    if (!m.has_proto_version())
    {
        fprintf(stderr, "S1 proto missing version!\n");
        return;
    }
    if (!m.proto_version().has_app_name() ||
        !m.proto_version().has_version()  ||
        !m.proto_version().has_b64variant())
    {
        fprintf(stderr, "S1 proto missing required version fields!\n");
        return;
    }
    fprintf(stderr, "starting decode from %s:%d\n",
            m.proto_version().app_name().c_str(),
            m.proto_version().version());
    Base64Variant  variant = (Base64Variant) m.proto_version().b64variant();
    if (!b64.set_variant(variant))
        fprintf(stderr, "ERROR INVALID Base64 VARIANT SPECIFIED (%d)\n",
                variant);
    else
        got_version = true;
}

bool uuz :: handle_s2_file_info(void)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR GOT S2 WITHOUT S1\n");
        return false;
    }
    if (m.type() != PFK::uuz::uuz_FILE_INFO)
    {
        fprintf(stderr, "S2 proto type mismatch\n");
        return false;
    }
    if (!m.has_file_info() ||
        !m.file_info().has_file_name()  ||
        !m.file_info().has_file_size())
    {
        fprintf(stderr, "S2 required fields missing\n");
        return false;
    }
    compression = PFK::uuz::NO_COMPRESSION;
    if (m.file_info().has_compression())
        compression = m.file_info().compression();
    output_filename = m.file_info().file_name();
    output_filesize = m.file_info().file_size();

    if (m.file_info().has_file_mode())
        output_filemode = m.file_info().file_mode();
    else
        output_filemode = 0;

    final_output_filename = output_filename;
    output_filename += ".TEMP_OUTPUT";

    if (m.file_info().has_encryption()   &&
        m.file_info().encryption() != PFK::uuz::NO_ENCRYPTION)
    {
        if (opts.encryption_key.size() == 0)
        {
            fprintf(stderr, "ERROR: input encrypted but no key provided!\n");
            exit(1);
        }
    }

    switch (compression)
    {
    case PFK::uuz::NO_COMPRESSION:
        fprintf(stderr, "starting file %s (%" PRIu64
                " bytes) with no compression\n",
                final_output_filename.c_str(), (uint64_t) output_filesize);
        break;
    case PFK::uuz::LIBZ_COMPRESSION:
        fprintf(stderr, "starting file %s (%" PRIu64
                " bytes) with LIBZ compression\n",
                final_output_filename.c_str(), (uint64_t) output_filesize);
        break;
    }

    bool dir_fail = false;
    {
        std::vector<std::string>  components;
        splitString(components, output_filename);
        std::string dirToMake;

        for (int ind = 0; ind < (components.size()-1); ind++)
        {
            dirToMake += components[ind];
            if (mkdir(dirToMake.c_str(), 0700) < 0)
            {
                int e = errno;
                if (e == EEXIST)
                {
                    // this is OK
                }
                else
                {
                    printf("ERROR: mkdir '%s': %d (%s)\n",
                           dirToMake.c_str(), e, strerror(e));
                    dir_fail = true;
                    break;
                }
            }
            dirToMake += "/";
        }
    }

    if (dir_fail)
        return false;

    output_f = fopen(output_filename.c_str(), "w");
    if (!output_f)
    {
        int e = errno;
        fprintf(stderr, "failed to open output filename: %d (%s)\n",
                e, strerror(e));
        return false;
    }
    else
    {
        if (output_filemode != 0)
        {
            chmod(output_filename.c_str(), S_IWUSR);
        }
    }
    expected_pos = 0;
    return true;
}

void uuz :: handle_s3_data(void)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR GOT S3 WITHOUT S1\n");
        return;
    }
    if (!output_f)
    {
        fprintf(stderr, "ERROR GOT S3 BUT FILE ISN'T OPEN\n");
        return;
    }
    if (m.type() != PFK::uuz::uuz_FILE_DATA)
    {
        fprintf(stderr, "S3 BUT PROTO TYPE MISMATCH\n");
        return;
    }
    if (!m.has_file_data() ||
        !m.file_data().has_position() ||
        !m.file_data().has_data())
    {
        fprintf(stderr, "S3 MISSING REQUIRED FIELDS\n");
        return;
    }

    const PFK::uuz::FileData &fd = m.file_data();
    const std::string &fdbuf = fd.data();

    if (fd.position() != expected_pos)
    {
        fprintf(stderr, "S3 INCORRECT POSITION (%" PRIu64 " != %" PRIu64 ")\n",
                (uint64_t) fd.position(), (uint64_t) expected_pos);
        return;
    }
    expected_pos += fd.data_size();

    if (compression == PFK::uuz::NO_COMPRESSION)
    {
        if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
        {
            // construct hmac key
            //   encryption key + filename + position
            std::ostringstream hmac_key_str;
            hmac_key_str << opts.encryption_key
                         << final_output_filename
                         << fd.position();
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
                                   (const unsigned char *) fdbuf.c_str(),
                                   fdbuf.size());
            DEBUGHMAC("HMAC update with %" PRIu32
                      " bytes %08x %08x %08x %08x\n",
                      (uint32_t) fdbuf.size(),
                      ((uint32_t*)fdbuf.c_str())[0],
                      ((uint32_t*)fdbuf.c_str())[1],
                      ((uint32_t*)fdbuf.c_str())[2],
                      ((uint32_t*)fdbuf.c_str())[3]);
            std::string hmac;
            hmac.resize(md_size);
            mbedtls_md_hmac_finish(&hmac_ctx,
                                   (unsigned char*) hmac.c_str());
            DEBUGHMAC("HMAC calculated %08x %08x %08x %08x\n",
                      ((uint32_t*)hmac.c_str())[0],
                      ((uint32_t*)hmac.c_str())[1],
                      ((uint32_t*)hmac.c_str())[2],
                      ((uint32_t*)hmac.c_str())[3]);
            if (hmac != fd.hmac())
            {
                printf("ERROR: HMAC MISMATCH! check your keys\n");
                exit(1);
            }
            mbedtls_md_hmac_reset(&hmac_ctx);
        }

        decrypt(obuf, fdbuf);
        fwrite(obuf, fd.data_size(), 1, output_f);
        mbedtls_md_update(&md_ctx, obuf, fd.data_size());
    }
    else if (compression == PFK::uuz::LIBZ_COMPRESSION)
    {
        if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
        {
            // construct hmac key
            //   encryption key + filename + position
            std::ostringstream hmac_key_str;
            hmac_key_str << opts.encryption_key
                         << final_output_filename
                         << fd.position();
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
                                   (const unsigned char*) fdbuf.c_str(),
                                   fdbuf.size());
            DEBUGHMAC("HMAC update with %" PRIu32
                      " bytes %08x %08x %08x %08x\n",
                      (uint32_t) fdbuf.size(),
                      ((uint32_t*)fdbuf.c_str())[0],
                      ((uint32_t*)fdbuf.c_str())[1],
                      ((uint32_t*)fdbuf.c_str())[2],
                      ((uint32_t*)fdbuf.c_str())[3]);
            std::string hmac;
            hmac.resize(md_size);
            mbedtls_md_hmac_finish(&hmac_ctx,
                                   (unsigned char*) hmac.c_str());
            DEBUGHMAC("HMAC calculated %08x %08x %08x %08x\n",
                      ((uint32_t*)hmac.c_str())[0],
                      ((uint32_t*)hmac.c_str())[1],
                      ((uint32_t*)hmac.c_str())[2],
                      ((uint32_t*)hmac.c_str())[3]);
            if (hmac != fd.hmac())
            {
                printf("ERROR: HMAC MISMATCH! check your keys\n");
                exit(1);
            }
            mbedtls_md_hmac_reset(&hmac_ctx);
        }

        decrypt(ibuf, fdbuf);
        zs.next_in = (Bytef*) ibuf;
        zs.avail_in = fd.data_size();

    again:
        zs.next_out = (Bytef*) obuf;
        zs.avail_out = BUFFER_SIZE;

        DEBUGLIBZ("before inflate, in %d out %d ; ",
               zs.avail_in, zs.avail_out);
        int cc = inflate(&zs, Z_NO_FLUSH);

        int produced = BUFFER_SIZE - zs.avail_out;
        DEBUGLIBZ("after inflate ret %d (%s) in %d out %d produced %d\n",
                  cc, deflateretstring(cc),
                  zs.avail_in, zs.avail_out, produced);

        if (produced > 0)
        {
            fwrite(obuf, produced, 1, output_f);
            mbedtls_md_update(&md_ctx, obuf, produced);
        }

        if (cc < 0 && cc != Z_BUF_ERROR)
        {
            fprintf(stderr, "ERROR: zlib failed decompression "
                    "with error %d (%s)\n",
                    cc, deflateretstring(cc));
            exit(1);
        }

        if (zs.avail_in != 0)
            goto again;
    }
}

void uuz :: handle_s9_complete(void)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR GOT S9 WITHOUT S1\n");
        return;
    }
    if (!output_f)
    {
        fprintf(stderr, "GOT S9 WITH NO FILE OPEN\n");
        return;
    }
    if (m.type() != PFK::uuz::uuz_FILE_COMPLETE)
    {
        fprintf(stderr, "S9 BUT PROTO MSG TYPE MISMATCH\n");
        return;
    }
    if (!m.has_file_complete() ||
        !m.file_complete().has_compressed_size())
    {
        fprintf(stderr, "S9 MISSING REQUIRED FIELDS\n");
        return;
    }
    size_t compressed_size = m.file_complete().compressed_size();

    if (compression == PFK::uuz::LIBZ_COMPRESSION)
    {
        zs.next_in = NULL;
        zs.avail_in = 0;

    again:
        zs.next_out = (Bytef*) obuf;
        zs.avail_out = BUFFER_SIZE;

        int cc = inflate(&zs, Z_FINISH);

        int produced = BUFFER_SIZE - zs.avail_out;
        if (produced > 0)
        {
            fwrite(obuf, produced, 1, output_f);
            mbedtls_md_update(&md_ctx, obuf, produced);
        }

        if (cc != Z_STREAM_END)
        {
            if (zs.avail_in != 0)
                goto again;
            printf("got cc %d\n", cc);
        }
    }

    off_t  output_file_pos = ftello(output_f);
    if (output_f)
    {
        fclose(output_f);
        output_f = NULL;
        if (rename(output_filename.c_str(),
                   final_output_filename.c_str()) < 0)
        {
            int e = errno;
            fprintf(stderr, "renaming file filename: %d (%s)\n",
                    e, strerror(e));
        }
        if (output_filemode != 0)
        {
            chmod(final_output_filename.c_str(), output_filemode);
        }
    }

    if (compression == PFK::uuz::LIBZ_COMPRESSION  &&
        expected_pos != compressed_size)
    {
        fprintf(stderr, "ERROR missing compressed data? "
                "(%" PRIu64 " != %" PRIu64 ")\n",
                (uint64_t) expected_pos, (uint64_t) compressed_size);
        return;
    }

    if ((size_t) output_file_pos != output_filesize)
    {
        fprintf(stderr, "ERROR missing data? "
                "(%" PRIu64 " != %" PRIu64 ")\n",
                (uint64_t) output_file_pos,
                (uint64_t) output_filesize);
        return;
    }

    {
        // if we made it this far with HMAC turned on,
        // then HMAC must be good too.
        if (opts.hmac != PFK::uuz::NO_HMAC)
            printf("HMAC match!\n");

        std::string sha;
        sha.resize(md_size);
        mbedtls_md_finish(&md_ctx, (unsigned char*) sha.c_str());
        if (sha != m.file_complete().sha())
            printf("ERROR: SHA256 MISMATCH\n");
        else
            printf("SHA256 match!\n");
    }

    fprintf(stderr, "decode successful\n");
}

void uuz :: decrypt(unsigned char *out, const std::string &in)
{
    switch (opts.encryption)
    {
    case PFK::uuz::NO_ENCRYPTION:
        memcpy(out, in.c_str(), in.size());
        break;

    case PFK::uuz::AES256_ENCRYPTION:
    {
        int sz = in.size();
        if (sz & 15)
        {
            fprintf(stderr, "ERROR: encrypted block is not "
                    "multiple of 16\n");
            return;
        }
        int blocks = sz / 16;

        unsigned char *outp = out;
        const unsigned char *inp = (const unsigned char *) in.c_str();

        for (int ind = 0; ind < blocks; ind++)
        {
            mbedtls_aes_crypt_ecb(
                &aes_ctx, MBEDTLS_AES_DECRYPT,
                inp, outp);

            inp += 16;
            outp += 16;
        }

        break;
    }
    }
}


}; // namespace PFK_uuz
