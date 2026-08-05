#ifndef __VITA_BITBUILDER_H__
#define __VITA_BITBUILDER_H__ 1

#include <cstdint>
#include <cstring>

// write a simple c++ class for a buffer builder that builds a buffer
// of data a few bits at a time.

// it should work like this: the buffer is prefilled with zeros. each
// time you call add_bits, it should start at the highest bit (7) of
// the first byte and work down towards 0. it should always add
// "bitwidth" bits, consuming that many bits from "value" and
// advancing the current position through the buffer. it should return
// true if adding was successful, or false if the starting_length has
// been consumed and there wasn't enough bits left. get_length should
// return false if we overflowed, or true if everything fit. it should
// also return in the pass-by-ref arguments the number of bytes
// consumed (including partially consumed bytes) and the number of
// bits that were consumed in the last byte.

class BitBuilder {
private:
    uint8_t* m_buffer;
    uint32_t m_capacity_bytes;
    uint32_t m_capacity_bits;
    uint32_t m_current_bit;
    bool m_failed;

public:
    BitBuilder(uint8_t* buffer, uint32_t starting_length)
        : m_buffer(buffer), 
          m_capacity_bytes(starting_length), 
          m_capacity_bits(starting_length * 8), 
          m_current_bit(0), 
          m_failed(false) 
    {
        // Protect against null pointers or invalid lengths
        if (m_buffer != nullptr && m_capacity_bytes > 0)
        {
            std::memset(m_buffer, 0, m_capacity_bytes);
        }
        else
        {
            m_failed = true;
        }
    }

    bool add_bits(uint64_t value, uint32_t bitwidth)
    {
        // Sticky failure check
        if (m_failed)
        {
            return false;
        }

        // Validate bitwidth (can't exceed the value size)
        if (bitwidth > 64)
        {
            m_failed = true;
            return false;
        }

        // Check if we have enough space left in the buffer
        if (m_current_bit + bitwidth > m_capacity_bits)
        {
            m_failed = true;
            return false;
        }

        // Write bits from highest requested bit down to 0
        for (int i = bitwidth - 1; i >= 0; --i)
        {
            // Extract the specific bit from the value
            bool bit = (value >> i) & 1;
            
            if (bit)
            {
                int byte_idx = m_current_bit / 8;
                 // Start at bit 7, work down to 0
                int bit_idx = 7 - (m_current_bit % 8);

                m_buffer[byte_idx] |= (1 << bit_idx);
            }
            m_current_bit++;
        }

        return true;
    }

    bool get_length(uint32_t &bytes, uint32_t &bits)
    {
        if (m_failed)
        {
            return false;
        }

        if (m_current_bit == 0)
        {
            bytes = 0;
            bits = 0;
        }
        else
        {
            // Calculate total bytes (rounding up for partially filled bytes)
            bytes = (m_current_bit + 7) / 8;

            // Calculate bits used in the last byte
            bits = m_current_bit % 8;
            if (bits == 0)
            {
                // If it divides evenly by 8,
                // all 8 bits of the last byte were used
                bits = 8; 
            }
        }
        
        return true;
    }
};

#endif // __VITA_BITBUILDER_H__
