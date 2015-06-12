#include <cstdio>
#include <cassert>
#include <thread>
#include <concurrent_queue.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

class BitStream {
private:
    static const int BUFSIZE = 4096;
    uint8_t buf[BUFSIZE];
    int pos_byte, pos_bit;
    FILE *fp = NULL;
public:
    ~BitStream();
    void open(const char *filename);
    uint32_t nextbits(int count, bool pop = true);
    void align();
};

struct Pixel {
    uint8_t r, g, b;
};

struct MPEG1Data {
    BitStream stream;
    uint32_t next_start_code;
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