#ifndef __VITA49_H__
#define __VITA49_H__ 1

#include "vita_bitbuilder.h"

#include <cmath>

struct Vita49Header {

    // packet types : table 5.1.1-1
    static const uint8_t PACKET_TYPE_SIGNAL = 1; // with stream identifier
    static const uint8_t PACKET_TYPE_CONTEXT = 4;
    // other packet types exist...

    uint8_t packet_type;

    bool class_present;

    // table 5.1.1-2
    static const uint8_t TSI_UTC = 1; // others exist
    uint8_t tsi; // timestamp integer format

    // table 5.1.1-3
    static const uint8_t TSF_PICO = 2; // others exist
    uint8_t tsf; // timestamp fractional format

    bool not_v490_pi; // 0 means V49.0 parsable
    uint8_t packet_count; // sequence# 0-15
    uint16_t packet_size; // in words

    uint32_t stream_id;
    uint32_t class_id;
    uint32_t timestamp_seconds;
    uint64_t timestamp_picoseconds;

    // signal specific
    bool trailer_present;
    bool spectrum_indicator;

    // context specific
    bool tsmode; // 0 means tsf is precise
    // cif0 stuff
    bool changed;
    bool bandwidth_present;
    double bandwidth;
    bool rf_ref_freq_present;
    double freq;
    bool gain_present;
    float gain2;
    float gain1;
    bool samplerate_present;
    double sample_rate;
    bool payloadformat_present;
    bool complex;
    // real for adc, complex cartesian for ddc
    // code 0 for signed fix-point

    void incr_packet_count(void)
    {
        packet_count = (packet_count + 1) & 0xF;
    }

    Vita49Header(bool _signal=true)
    {
        if (_signal)
        {
            packet_type = PACKET_TYPE_SIGNAL;
            trailer_present = false;
        }
        else
        {
            packet_type = PACKET_TYPE_CONTEXT;
            tsmode = 1; // timestamp is general
        }
        spectrum_indicator = 0;
        tsi = TSI_UTC;
        tsf = TSF_PICO;
        not_v490_pi = 0;
        class_present = false;
        packet_count = 0;
        packet_size = 0;

        // context stuff
        changed = false;
        bandwidth_present = false;
        rf_ref_freq_present = false;
        gain_present = false;
        samplerate_present = false;
        payloadformat_present = false;
    }

    // this is a "Q9.7" fixed-point representation.
    void encode_q9_7(BitBuilder &bb, double v)
    {
        // 1. scale by 2^7 (128.0) to align the radix point.
        double scaled = v * 128.0;

        // 2. Convert to 16-bit 2's complement integer.
        int16_t fixed = static_cast<int16_t>(std::round(scaled));

        // 3. Cast to unsigned to ensure safe extraction of
        //    raw bit patterns.
        uint16_t raw_bits = static_cast<uint16_t>(fixed);

        // 4. add the bits.
        bb.add_bits(raw_bits, 16);
    }

    // this is a "Q44.20" fixed-point representation.
    void encode_q44_20(BitBuilder &bb, double v)
    {
        // 1. Scale the double by 2^20 to align the radix point
        //    (note 2^20 = 1048576.0).
        double scaled = v * 1048576.0;

        // 2. Convert to 64-bit two's complement integer.
        //    Rounding is optional but usually a good idea
        //    for fractional fixed-point
        int64_t fixed = static_cast<int64_t>(std::round(scaled));

        // 3. Safely cast to unsigned 64-bit to prevent
        //    arithmetic shift issues when shifting
        //    and masking the bits
        uint64_t raw_bits = static_cast<uint64_t>(fixed);

        // 4. Split into the first 32-bit word (MSW)
        //    and second 32-bit word (LSW)
        uint32_t word1 = static_cast<uint32_t>(raw_bits >> 32);
        uint32_t word2 = static_cast<uint32_t>(raw_bits & 0xFFFFFFFF);

        // 5. add the bits in order.
        bb.add_bits(word1, 32);
        bb.add_bits(word2, 32);
    }

    // for signal type, this only encodes the header up through the
    // timestamp; body is appended by the caller.
    // for context type, this encodes the whole packet.
    int encode(unsigned char *buffer, uint32_t size)
    {
        BitBuilder   bb(buffer, size);

        packet_size = 0;

// see 6.1
// signal bits:  4-type 1-C 1-T 1-r 1-S 2-TSI 2-TSF 4-count 16-size

// see 7.1, 7.1.4
// context bits: 4-type 1-C 1-R 1-nd0 1-TSMode 2-TSI 2-TSM 4-count 16-size
        bb.add_bits(packet_type             ,  4);
        bb.add_bits(class_present      ? 1:0,  1);

        if (packet_type == PACKET_TYPE_SIGNAL)
        {
            bb.add_bits(trailer_present    ? 1:0,  1);
            bb.add_bits( /*reserved*/          0,  1);
            bb.add_bits(spectrum_indicator ? 1:0,  1);
        }
        else if (packet_type == PACKET_TYPE_CONTEXT)
        {
            bb.add_bits( /*reserved*/          0,  1);
            bb.add_bits(not_v490_pi        ? 1:0,  1);
            bb.add_bits(tsmode                  ,  1);
        }
        else
            return -1;

        bb.add_bits(tsi                     ,  2);
        bb.add_bits(tsf                     ,  2);
        bb.add_bits(packet_count            ,  4);

        // NOTE this will be patched at the end of building
        // the packet.
        bb.add_bits(packet_size             , 16);
        packet_size++;

//   then 32-stream  64-class  32-tsi   64-tsf

        bb.add_bits(stream_id               , 32);
        packet_size++;

        if (class_present)
        {
            bb.add_bits(class_id            , 64);
            packet_size += 2;
        }

        bb.add_bits(timestamp_seconds       , 32);
        packet_size++;

        bb.add_bits(timestamp_picoseconds   , 64);
        packet_size += 2;

        if (packet_type == PACKET_TYPE_CONTEXT)
        {
            // cif0 see 9.1
            bb.add_bits(changed ? 1:0, 1); // 31
            bb.add_bits(/*ref point id*/ 0, 1); // 30
            bb.add_bits(bandwidth_present ? 1:0, 1); // 29
            bb.add_bits(/*if ref freq*/ 1, 1); // 28
            bb.add_bits(rf_ref_freq_present ? 1:0, 1); // 27
            bb.add_bits(/*not supported*/ 0, 3); // 26-24
            bb.add_bits(gain_present ? 1:0, 1); // 23
            bb.add_bits(/*not supported*/ 0, 1); // 22
            bb.add_bits(samplerate_present ? 1:0, 1); // 21
            bb.add_bits(/*not supported*/ 0, 5); // 20-16
            bb.add_bits(payloadformat_present ? 1:0, 1); // 15
            bb.add_bits(/*not supported*/ 0, 15); // 14-0
            packet_size++;

            if (bandwidth_present)
            {
                // see 9.5.1.
                encode_q44_20(bb, bandwidth);
                packet_size += 2;
            }
            if (true /* IF ref frequency*/)
            {
                // see 9.5.5
                // IF ref freq should be set to 0 for a DDC.
                encode_q44_20(bb, 0);
                packet_size += 2;
            }
            if (rf_ref_freq_present)
            {
                // see 9.5.10
                encode_q44_20(bb, freq);
                packet_size += 2;
            }
            if (gain_present)
            {
                // see 9.5.3
                encode_q9_7(bb, gain2);
                encode_q9_7(bb, gain1);
                packet_size += 1;
            }
            if (samplerate_present)
            {
                // see 9.5.12
                encode_q44_20(bb, sample_rate);
                packet_size += 2;
            }
            if (payloadformat_present)
            {
                // see 9.13.3
                bb.add_bits(/*pack*/0, 1);
                bb.add_bits(/*complex cartesian*/1, 2);
                bb.add_bits(/*data item format signed fixed-point*/0, 5);
                bb.add_bits(/*repeat*/0, 1);
                bb.add_bits(/*event-tag size*/0, 3);
                bb.add_bits(/*channel-tag size*/0, 4);
                bb.add_bits(/*data-item frac size*/0, 4);
                bb.add_bits(/*item packet field size-1*/15, 6);
                bb.add_bits(/*data item size-1*/15, 6);
                bb.add_bits(/*repeat count*/0, 16);
                bb.add_bits(/*vector size*/0, 16);
                packet_size += 2;
            }
        }

        // now that building is complete, patch the length
        // back into the first word.
        buffer[2] = (packet_size >> 8) & 0xFF;
        buffer[3] = (packet_size >> 0) & 0xFF;

        uint32_t return_size, return_bits;
        if (!bb.get_length(return_size, return_bits))
            return -1;
        return (int) return_size;
    }
};

#endif // __VITA49_H__
