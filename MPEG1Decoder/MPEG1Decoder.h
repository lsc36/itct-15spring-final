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
    BitStream *m_stream;
    unsigned m_len = 0;
    VLCTableEntry *m_lookup = NULL;
public:
    ~VLCTable();
    void setStream(BitStream &stream);
    void buildTable(std::vector<VLCTableEntry> &table);
    int get();
};

struct Tables {
    static VLCTable macro_addrinc;
    static VLCTable macro_I;
    static VLCTable macro_P;
    static VLCTable macro_B;
    static VLCTable cbp;
    static VLCTable mv;
    static VLCTable dct_dc_luma;
    static VLCTable dct_dc_chroma;
    static VLCTable dct_coeff_first;
    static VLCTable dct_coeff_next;
    static void initTables(BitStream &stream);
};

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