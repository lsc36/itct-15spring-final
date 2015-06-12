#include "MPEG1Decoder.h"

BitStream::~BitStream()
{
    if (fp != NULL) fclose(fp);
}

void BitStream::open(const char *filename)
{
    fp = fopen(filename, "rb");
    assert(fp != NULL);
    fread(buf, 1, BUFSIZE, fp);
    pos_byte = 0;
    pos_bit = 7;
}

uint32_t BitStream::nextbits(int count, bool pop)
{
    // TODO: read data to buffer
    uint32_t ret = 0;
    int left = count;
    int npos_byte = pos_byte, npos_bit = pos_bit;
    if (npos_bit != 7 && left > npos_bit) {
        ret = buf[npos_byte] & ((1 << (npos_bit + 1)) - 1);
        left -= npos_bit + 1;
        npos_byte++;
        npos_bit = 7;
    }
    while (left >= 8) {
        ret = ret << 8 | buf[npos_byte];
        left -= 8;
        npos_byte++;
    }
    if (left) {
        ret = ret << left | (buf[npos_byte] >> (npos_bit - left + 1) & ((1 << left) - 1));
        npos_bit -= left;
    }
    if (pop) {
        pos_byte = npos_byte;
        pos_bit = npos_bit;
    }
    return ret;
}

void BitStream::align()
{
    if (pos_bit != 7) {
        pos_byte++;
        pos_bit = 7;
    }
}