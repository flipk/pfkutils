
#ifndef __UUZ_OPTIONS_H__
#define __UUZ_OPTIONS_H__

#include <string>
#include <sys/stat.h>
#include <zlib.h>
#include <vector>
#include UUZ_PROTO_HDR
#include "newbase64.h"

namespace PFK_uuz {

#define DEBUGPROTO(x...)  if (opts.debug &  1) fprintf(stderr, x)
#define DEBUGLIBZ(x...)   if (opts.debug &  2) fprintf(stderr, x)
#define DEBUGDECODE(x...) if (opts.debug &  4) fprintf(stderr, x)
#define DEBUGMBED(x...)   if (opts.debug &  8) fprintf(stderr, x)
#define DEBUGHMAC(x...)   if (opts.debug & 16) fprintf(stderr, x)
#define DEBUGFLAG_HMAC    (opts.debug & 16)

struct uuzopts {
    bool           _ok;

    struct input_file {
        std::string   path;
        struct stat   sb;
        int           fd;
        FILE         *f;
    };
    std::vector<input_file*>  input_files;

    input_file    *current_file;
    uint32_t       debug;
    bool           text_headers;

    std::string    output_uuz;
    FILE          *uuz_f;
    bool           file_splitting_enabled;
    size_t         outfile_split_size;
    int            outfile_counter;
    void           check_rollover(void);

    enum {  NONE, ENCODE, DECODE } mode;
    PFK::uuz::CompressionSetting   compression;
    PFK::uuz::EncryptionSetting    encryption;
    std::string                    encryption_key;
    PFK::uuz::HMACSetting          hmac;
    Base64Variant                  data_block_variant;

    static const PFK::uuz::CompressionSetting compression_default =
        PFK::uuz::LIBZ_COMPRESSION;
    static const Base64Variant data_block_variant_default =
        BASE64_GNUBASH;

    uuzopts(int argc, char ** argv);
    ~uuzopts(void);
    operator bool() const { return _ok; }
    void usage(void);
};

static inline const char * flushstring(int flush)
{
    switch (flush)
    {
    case Z_NO_FLUSH: return "Z_NO_FLUSH";
    case Z_PARTIAL_FLUSH: return "Z_PARTIAL_FLUSH";
    case Z_SYNC_FLUSH: return "Z_SYNC_FLUSH";
    case Z_FULL_FLUSH: return "Z_FULL_FLUSH";
    case Z_FINISH:   return "Z_FINISH";
    case Z_BLOCK: return "Z_BLOCK";
    case Z_TREES: return "Z_TREES";
    default: ;
    }
    return "UNKNOWN FLUSH VALUE";
}
static inline const char * deflateretstring(int cc)
{
    switch (cc) {
    case Z_OK:  return "Z_OK";
    case Z_STREAM_END: return "Z_STREAM_END";
    case Z_NEED_DICT: return "Z_NEED_DICT";
    case Z_ERRNO: return "Z_ERRNO";
    case Z_STREAM_ERROR: return "Z_STREAM_ERROR";
    case Z_DATA_ERROR: return "Z_DATA_ERROR";
    case Z_MEM_ERROR: return "Z_MEM_ERROR";
    case Z_BUF_ERROR: return "Z_BUF_ERROR";
    case Z_VERSION_ERROR: return "Z_VERSION_ERROR";
    default: ;
    }
    return "UNKNOWN RETURN VALUE";
}


}; // namespace PFK_uuz

#endif /* __UUZ_OPTIONS_H__ */
