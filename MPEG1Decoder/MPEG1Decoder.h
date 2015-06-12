#include <cstdio>
#include <cassert>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct MPEG1Data {
    FILE *fp;
    int width;
    int height;
    double pixel_ar;
    double fps;
    uint8_t q_intra[64];
    uint8_t q_nonintra[64];
};

void decodeHeader(MPEG1Data &mpg);