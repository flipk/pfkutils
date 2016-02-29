#if 0
set -e -x
g++ readWav.cc -o readWav
./readWav < ringing.wav 5> ringing.wav.txt
exit 0
#endif

#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

struct riff_header {
    uint32_t  riff;
    uint32_t cksize;
    uint32_t  wave;
};

struct chunk_header {
    uint32_t  chunk_id;
    uint32_t  chunk_len;
};

struct fmt_chunk {
    uint16_t  format_code;
    uint16_t  num_interleaved_channels; // i.e. 2 = stereo
    uint32_t  samples_per_second;
    uint32_t  bytes_per_second;
    uint16_t  data_block_size; // (i.e. 4 bytes if 2 channels)
    uint16_t  bits_per_sample; // (i.e. 16 = bits per channel)
    // if short form (16 bytes, stops here)
    uint16_t  size_of_extension; // 0 or 22
    uint16_t  valid_bits_per_sample;
    uint32_t  speaker_position_mask;
    uint8_t  guid[16];
};

const char * wav_format_string(int format_code) {
    switch (format_code) {
    case 1: return "PCM";
    case 3: return "IEEE_FLOAT";
    case 6: return "ALAW";
    case 7: return "ULAW";
    case 0xFFFE: return "EXTENSIBLE";
    default: 
        ;
    }
    return "UNKNOWN";
}

int
main()
{
    struct riff_header rh;
    struct fmt_chunk  fmt;
    FILE * exportFile;

    exportFile = fdopen(5, "w");

    if (read(0, &rh, 12) != 12)
        return 1;

    while (1)
    {
        struct chunk_header ch;
        if (read(0, &ch, 8) != 8)
            break;
        printf(" chunk type %08x ('%c%c%c%c') len %d \n",
               ch.chunk_id,
               (ch.chunk_id >> 0) & 0xFF,
               (ch.chunk_id >> 8) & 0xFF,
               (ch.chunk_id >> 16) & 0xFF,
               (ch.chunk_id >> 24) & 0xFF,
               ch.chunk_len);

        switch (ch.chunk_id)
        {
        case 0x20746d66: // 'fmt '
            if (read(0, &fmt, ch.chunk_len) != ch.chunk_len)
                return 3;
            printf("format %d (%s) chans %d samp/s %d B/s %d "
                   "blksz %d bits/samp %d\n",
                   fmt.format_code, wav_format_string(fmt.format_code),
                   fmt.num_interleaved_channels, fmt.samples_per_second,
                   fmt.bytes_per_second, fmt.data_block_size,
                   fmt.bits_per_sample);
            break;
        case 0x61746164: // 'data'
        {
            uint8_t  data[ch.chunk_len];
            if (read(0, data, ch.chunk_len) != ch.chunk_len)
                return 4;
            if (fmt.data_block_size != 4 ||
                fmt.bits_per_sample != 16)
            {
                printf("unsupported blksz or bits/samp\n");
            }
            else
            {
                if (fmt.num_interleaved_channels == 2)
                {
                    struct stereo_samples {
                        int16_t  left;
                        int16_t  right;
                    };
                    struct stereo_samples * samples =
                        (struct stereo_samples *) data;
                    for (int sample = 0;
                         sample < (ch.chunk_len / fmt.data_block_size);
                         sample++)
                    {
#if 0
                        printf("sample %d left %d right %d\n",
                               sample, samples[sample].left,
                               samples[sample].right);
#endif
                        fprintf(exportFile,
                                "%d %d %d\n",
                                sample, samples[sample].left,
                                samples[sample].right);
                    }
                }
                else
                {
                    printf("unsupported num channels\n");
                }
            }
            break;
        }
        default:
            lseek(0, ch.chunk_len, SEEK_CUR);
        }
    }

    return 0;
}
