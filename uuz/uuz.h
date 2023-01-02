
#ifndef __UUZ_H__
#define __UUZ_H__ 1

#include "uuz_options.h"
#include <zlib.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <google/protobuf/message.h>
#include UUZ_PROTO_HDR

namespace PFK_uuz {

#define PFK_UUZ_APP_NAME "uuz"

class uuz {
    uuzopts  opts;

    enum Scode_version_t {
        SCODE_VERSION       =  1,
        SCODE_FILE_INFO  = 2,
        SCODE_DATA  = 3,
        SCODE_COMPLETE = 9
    };

    // distance between newline chars is encoded line
    // plus length of "S%d" in front.
    static const int MAX_ENCODED_LINE_LENGTH = 76;
    // translate max line length into # binary bytes (57).
    static const int MAX_BINARY_LINE_LENGTH = 76 / 4 * 3;
    static const int BUFFER_SIZE = 4096;
    // for pb serialization of FileData, add 10% overhead.
    static const int SERIALIZED_BUFSIZE = 4096 * 11 / 10;
    // 3 bytes becomes 4 when base64 encoded.
    static const int B64_ENCODED_PBSIZE = SERIALIZED_BUFSIZE * 4 / 3;
    unsigned char ibuf[BUFFER_SIZE];
    unsigned char obuf[BUFFER_SIZE];

    Base64 b64;
    z_stream zs;
    mbedtls_aes_context   aes_ctx;
    mbedtls_md_context_t  hmac_ctx;
    mbedtls_md_context_t  md_ctx;
    int                   md_size;

    PFK::uuz::uuzMsg   m;
    std::string   serialized_pb;
    std::string   b64_encoded_pb;

    void format_hexbytes(std::string &out, const std::string &in);

    bool uuz_encode_emit_s1(void);
    int uuz_encode(void);
    void encrypt(std::string &out, unsigned char *in, int ilen);
    bool encode_m(int Scode);

    std::string   partial_serialized_pb;
    bool got_version;
    std::string   final_output_filename;
    std::string   output_filename;
    size_t        output_filesize;
    uint32_t      output_filemode; // 0 means not specified
    FILE         *output_f;
    PFK::uuz::CompressionSetting  compression;
    size_t        expected_pos;

    typedef enum {
        DECODE_ERR,
        DECODE_COMPLETE,
        DECODE_MORE
    } uuz_decode_ret_t;
    uuz_decode_ret_t uuz_decode(void);
    void decrypt(unsigned char *out, const std::string &in);
    bool getline(void);
    enum decode_b64_res_t { INVALID_B64, PARTIAL_B64, COMPLETE_B64 };
    bool decode_version(std::string &out, const std::string &in);
    decode_b64_res_t decode_b64(char s, int Scode);
    bool decode_m(void);
    void handle_s1_version(void);
    void handle_s2_file_info(void);
    void handle_s3_data(void);
    void handle_s9_complete(void);

public:
    uuz(int argc, char ** argv);
    ~uuz(void);
    operator bool() const { return opts; }
    int main(void);
}; // class uuz

}; // namespace PFK_uuz

#endif /* __UUZ_H__ */
