
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
uuz :: uuz_decode(bool list_only)
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
            if (decode_m(Scode))
                switch (Scode)
                {
                case SCODE_VERSION:
                    handle_s1_version(list_only);
                    break;

                case SCODE_FILE_INFO:
                    if (handle_s2_file_info(list_only) == false)
                        done = true;
                    break;

                case SCODE_DATA:
                    handle_s3_data(list_only);
                    break;

                case SCODE_COMPLETE:
                    handle_s9_complete(list_only);
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

bool uuz:: decode_m(int Scode)
{
    google::protobuf::Message * msg_to_decode = &m;

    if (Scode != SCODE_VERSION)
    {
        if (encryption != PFK::uuz::NO_ENCRYPTION)
        {
            msg_to_decode = &encrypted_container;
        }
    }

    msg_to_decode->Clear();
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
            bool r = msg_to_decode->ParseFromCodedStream(&cis);
            cis.PopLimit(old);
            if (!r)
            {
                fprintf(stderr, "failed to decode stream of len %u\n",
                        len);
                return false;
            }
        } // destroy cis
    } // destroy ais

    DEBUGPROTO("parsed proto:\n%s\n",
               msg_to_decode->DebugString().c_str());

    if (Scode != SCODE_VERSION)
    {
        if (encryption == PFK::uuz::AES256_ENCRYPTION)
        {
            if (!encrypted_container.has_data() ||
                !encrypted_container.has_data_size() ||
                !encrypted_container.has_salt()  ||
                !encrypted_container.has_hmac())
            {
                fprintf(stderr, "ERROR encrypted data missing "
                        "required fields\n");
                return false;
            }

            if (encrypted_container.has_iv())
                iv = encrypted_container.iv();

            // make buffer big enough to decrypt with padding.
            serialized_pb.resize( encrypted_container.data().size()  );
            decrypt((unsigned char*) serialized_pb.c_str(),
                    encrypted_container.data());
            // then truncate the unused.
            serialized_pb.resize( encrypted_container.data_size() );

            if (opts.hmac == PFK::uuz::HMAC_SHA256HMAC)
            {
                // construct hmac key
                //   encryption key + data_size + salt
                std::ostringstream hmac_key_str;
                hmac_key_str << opts.encryption_key
                             << encrypted_container.data_size()
                             << encrypted_container.salt();
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
                                       encrypted_container.data_size());
                DEBUGHMAC("HMAC update with %" PRIu32 " bytes "
                          "%08x %08x %08x %08x\n",
                          (uint32_t) encrypted_container.data_size(),
                          ((uint32_t*)serialized_pb.c_str())[0],
                          ((uint32_t*)serialized_pb.c_str())[1],
                          ((uint32_t*)serialized_pb.c_str())[2],
                          ((uint32_t*)serialized_pb.c_str())[3]);

                std::string hmac;
                hmac.resize(md_size);
                mbedtls_md_hmac_finish(&hmac_ctx,
                                       (unsigned char*) hmac.c_str());
                DEBUGHMAC("HMAC calculated %08x %08x %08x %08x\n",
                          ((uint32_t*)hmac.c_str())[0],
                          ((uint32_t*)hmac.c_str())[1],
                          ((uint32_t*)hmac.c_str())[2],
                          ((uint32_t*)hmac.c_str())[3]);
                mbedtls_md_hmac_reset(&hmac_ctx);

                if (hmac != encrypted_container.hmac())
                {
                    fprintf(stderr, "ERROR: HMAC MISMATCH!\n");
                    exit(1);
                }
            }

            if (!m.ParseFromString(serialized_pb))
                return false;

            DEBUGPROTO("parsed proto:\n%s\n",
                       m.DebugString().c_str());
        }
    }

    return true;
}

void uuz :: handle_s1_version(bool list_only)
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
    if (m.proto_version().version() != PFK::uuz::uuz_VERSION_2)
    {
        fprintf(stderr, "S1 proto version %d not supported! (only %d)!\n",
                m.proto_version().version(), PFK::uuz::uuz_VERSION_2);
        return;
    }
    Base64Variant  variant = (Base64Variant) m.proto_version().b64variant();
    opts.data_block_variant = variant;
    if (!b64.set_variant(variant))
    {
        fprintf(stderr, "ERROR INVALID Base64 VARIANT SPECIFIED (%d)\n",
                variant);
        exit(1);
    }

    // defaults
    compression = PFK::uuz::NO_COMPRESSION;
    encryption = PFK::uuz::NO_ENCRYPTION;
    hmac = PFK::uuz::NO_HMAC;

    if (m.proto_version().has_compression())
        compression = m.proto_version().compression();
    if (m.proto_version().has_encryption())
        encryption = m.proto_version().encryption();
    if (m.proto_version().has_hmac())
        hmac = m.proto_version().hmac();

    compression_name = "no compression";
    switch (compression)
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

    if (encryption != PFK::uuz::NO_ENCRYPTION)
    {
        if (opts.encryption_key.size() == 0)
        {
            fprintf(stderr, "ERROR: input encrypted but no key provided!\n");
            exit(1);
        }
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

    got_version = true;
    if (list_only)
    {
        printf("VERSION_INFO: program \"%s\" protoversion %d "
               "b64variant %d (%s) %s %s %s\n",
               m.proto_version().app_name().c_str(),
               m.proto_version().version(),
               (int) opts.data_block_variant,
               Base64::variant_name(opts.data_block_variant),
               compression_name,
               encryption_name,
               hmac_name);
    }
    else
    {
        fprintf(stderr, "starting decode from %s:%d\n",
                m.proto_version().app_name().c_str(),
                m.proto_version().version());
    }
}

bool uuz :: handle_s2_file_info(bool list_only)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR: GOT S2 WITHOUT S1\n");
        exit(1);
    }
    if (m.type() != PFK::uuz::uuz_FILE_INFO)
    {
        fprintf(stderr, "ERROR: S2 proto type mismatch\n");
        exit(1);
    }
    if (!m.has_file_info() ||
        !m.file_info().has_file_name()  ||
        !m.file_info().has_file_size())
    {
        fprintf(stderr, "ERROR: S2 required fields missing\n");
        exit(1);
    }

    output_filename = m.file_info().file_name();
    output_filesize = m.file_info().file_size();

    if (m.file_info().has_file_mode())
        output_filemode = m.file_info().file_mode();
    else
        output_filemode = 0;

    final_output_filename = output_filename;
    output_filename += ".TEMP_OUTPUT";

    expected_pos = 0;
    output_file_pos = 0;
    if (list_only)
    {
        return true;
    }

    fprintf(stderr, "starting file %s (%" PRIu64
            " bytes) with %s %s %s\n",
            final_output_filename.c_str(), (uint64_t) output_filesize,
            compression_name, encryption_name, hmac_name);

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
    return true;
}

void uuz :: handle_s3_data(bool list_only)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR: GOT S3 WITHOUT S1\n");
        exit(1);
    }
    if (!output_f && !list_only)
    {
        fprintf(stderr, "ERROR: GOT S3 BUT FILE ISN'T OPEN\n");
        exit(1);
    }
    if (m.type() != PFK::uuz::uuz_FILE_DATA)
    {
        fprintf(stderr, "ERROR: S3 BUT PROTO TYPE MISMATCH\n");
        exit(1);
    }
    if (!m.has_file_data() ||
        !m.file_data().has_position() ||
        !m.file_data().has_data())
    {
        fprintf(stderr, "ERROR: S3 MISSING REQUIRED FIELDS\n");
        exit(1);
    }

    const PFK::uuz::FileData &fd = m.file_data();
    const std::string &fdbuf = fd.data();

    if (fd.position() != expected_pos)
    {
        fprintf(stderr, "ERROR: S3 POSITION MISMATCH, DATA MISSING? "
                "(%" PRIu64 " != %" PRIu64 ")\n",
                (uint64_t) fd.position(), (uint64_t) expected_pos);
        exit(1);
    }
    expected_pos += fd.data_size();

    if (compression == PFK::uuz::NO_COMPRESSION)
    {
        if (!list_only)
            fwrite(fdbuf.c_str(), fd.data_size(), 1, output_f);
        output_file_pos += fd.data_size();
        mbedtls_md_update(&md_ctx,
                          (const unsigned char *) fdbuf.c_str(),
                          fd.data_size());
    }
    else if (compression == PFK::uuz::LIBZ_COMPRESSION)
    {
        zs.next_in = (Bytef*) fdbuf.c_str();
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
            if (!list_only)
                fwrite(obuf, produced, 1, output_f);
            output_file_pos += produced;
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

void uuz :: handle_s9_complete(bool list_only)
{
    if (!got_version)
    {
        fprintf(stderr, "ERROR: GOT S9 WITHOUT S1\n");
        exit(1);
    }
    if (!output_f && !list_only)
    {
        fprintf(stderr, "ERROR: GOT S9 WITH NO FILE OPEN\n");
        exit(1);
    }
    if (m.type() != PFK::uuz::uuz_FILE_COMPLETE)
    {
        fprintf(stderr, "ERROR: S9 BUT PROTO MSG TYPE MISMATCH\n");
        exit(1);
    }
    if (!m.has_file_complete() ||
        !m.file_complete().has_compressed_size())
    {
        fprintf(stderr, "ERROR: S9 MISSING REQUIRED FIELDS\n");
        exit(1);
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
            if (!list_only)
                fwrite(obuf, produced, 1, output_f);
            output_file_pos += produced;
            mbedtls_md_update(&md_ctx, obuf, produced);
        }

        if (cc != Z_STREAM_END)
        {
            if (zs.avail_in != 0)
                goto again;
            printf("got cc %d\n", cc);
        }
    }

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
        exit(1);
    }

    if ((size_t) output_file_pos != output_filesize)
    {
        fprintf(stderr, "ERROR missing data? "
                "(%" PRIu64 " != %" PRIu64 ")\n",
                (uint64_t) output_file_pos,
                (uint64_t) output_filesize);
        exit(1);
    }

    bool sha_match = false;
    {
        std::string sha;
        sha.resize(md_size);
        mbedtls_md_finish(&md_ctx, (unsigned char*) sha.c_str());
        sha_match = (sha == m.file_complete().sha());
    }
    if (sha_match == false)
    {
        fprintf(stderr, "ERROR: SHA256 MISMATCH!\n");
        exit(1);
    }

    if (list_only)
    {
        ListInfo * li = new ListInfo;

        li->filename = final_output_filename;
        li->mode = output_filemode;
        li->size = output_filesize;
        if (m.file_complete().has_compressed_size()  &&
            compression != PFK::uuz::NO_COMPRESSION)
        {
            li->csize = m.file_complete().compressed_size();
            uint32_t percent_compressed =
                (m.file_complete().compressed_size() * 100) /
                output_filesize;
            li->percent = percent_compressed;
        }
        else
        {
            li->csize = 0;
            li->percent = 0;
        }
        if (hmac != PFK::uuz::NO_HMAC)
            li->hmac_status = " OK ";
        else
            li->hmac_status = "  - ";
        if (sha_match)
            li->sha_status = " OK ";
        else
            li->sha_status = "FAIL";

        list_output.push_back(li);
    }
    else
    {
        // if we made it this far with HMAC turned on,
        // then HMAC must be good too.
        if (hmac != PFK::uuz::NO_HMAC)
            fprintf(stderr, "HMAC match!\n");
        if (sha_match)
            fprintf(stderr, "SHA256 match!\n");
        else
        {
            fprintf(stderr, "ERROR: SHA256 MISMATCH\n");
            exit(1);
        }
        fprintf(stderr, "decode successful\n");
    }
}

void uuz :: decrypt(unsigned char *out, const std::string &in)
{
    switch (encryption)
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
            exit(1);
        }
        int blocks = sz / 16;

        unsigned char *outp = out;
        const unsigned char *inp = (const unsigned char *) in.c_str();

        if (DEBUGFLAG_MBED)
        {
            std::string  hex_iv;
            std::string  hex_in;
            format_hexbytes(hex_iv, iv);
            format_hexbytes(hex_in, in);
            fprintf(stderr, "AES decrypt %u bytes, iv before %s\n"
                    "inbytes %s\n",
                    mbed_aes_block_size * blocks,
                    hex_iv.c_str(),
                    hex_in.c_str());
        }

        mbedtls_aes_crypt_cbc(
            &aes_ctx, MBEDTLS_AES_DECRYPT,
            mbed_aes_block_size * blocks,
            (unsigned char *) iv.c_str(),
            (unsigned char *) in.c_str(),
            (unsigned char *) out);

        if (DEBUGFLAG_MBED)
        {
            std::string  hex_iv;
            std::string  hex_out;
            format_hexbytes(hex_iv, iv);
            format_hexbytes(hex_out, std::string((char*)out, in.size()));
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
