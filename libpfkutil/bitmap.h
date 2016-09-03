/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#ifndef __pfkbitmap_h__
#define __pfkbitmap_h__ 1

template <int numbits>
class Bitmap {
    static const int bits_per_word = sizeof(unsigned long) * 8;
    static const int digits_per_word = sizeof(unsigned long) * 2;
    static const int array_size = (numbits + bits_per_word - 1) / bits_per_word;
    static const unsigned long thebit = 1;
    unsigned long words[array_size];
public:
    Bitmap(void) { memset(words, 0, sizeof(words)); }
    bool set(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        words[word] |= (thebit << bit);
        return true;
    }
    bool clear(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        words[word] &= ~(thebit << bit);
        return true;
    }
    bool isset(int bitno) {
        if (bitno >= numbits)
            return false;
        int word = bitno / bits_per_word;
        int bit = bitno % bits_per_word;
        return ((words[word] & (thebit << bit)) != 0);
    }
    std::string print(void) {
        std::ostringstream str;
        str << "words: " << std::hex
            << std::setw(digits_per_word) << std::setfill('0');
        for (int word = 0; word < array_size; word++)
            str << words[word] << " ";
        str << std::endl;
        str << "bits: " << std::dec << std::setw(0);
        for (int bit = 0; bit < numbits; bit++)
            if (isset(bit))
                str << bit << " ";
        str << std::endl;
        return str.str();
    }
};

#endif /* __pfkbitmap_h__ */
