#include <cstdio>
#include <vector>
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
    int pos_file;
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

struct VLCTableEntry {
    unsigned code, len;
    int value;
};

class VLCTable {
private:
    static bool s_inited;
    BitStream &m_stream;
    unsigned m_len = 0;
    VLCTableEntry **m_lookup;
protected:
    static std::vector<VLCTableEntry> s_table;
    virtual void _buildTable() = 0;
public:
    VLCTable(BitStream &stream);
    ~VLCTable();
    void init();
    int get();
};

#include "vlctables.h"

struct PictureData {
    int temp_ref;
    char type;
    bool full_pel_forward;
    int forward_f, forward_f_size;
    bool full_pel_backward;
    int backward_f, backward_f_size;
};

struct SliceData {
    int vpos;
    int q_scale;
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
    PictureData cur_picture;
    SliceData cur_slice;
    MPEG1Data();
};

void decodeHeader(MPEG1Data &mpg);
void decodeGOP(MPEG1Data &mpg);