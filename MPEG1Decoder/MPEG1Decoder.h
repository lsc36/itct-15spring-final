#include <cstdio>
#include <cassert>
#include <thread>
#include <concurrent_queue.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

struct Pixel {
    uint8_t r, g, b;
};

struct MPEG1Data {
    FILE *fp;
    int width;
    int height;
    double pixel_ar;
    double fps;
    uint8_t q_intra[64];
    uint8_t q_nonintra[64];
    concurrency::concurrent_queue<Pixel*> frames;
};

void decodeHeader(MPEG1Data &mpg);
void decodeGOP(MPEG1Data &mpg);