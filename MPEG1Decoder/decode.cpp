#include "MPEG1Decoder.h"

double pixel_ar_table[16] = {0.0, 1.0, 0.6735, 0.7031, 0.7615, 0.8055, 0.8437, 0.8935, 0.9157, 0.9815, 1.0255, 1.0695, 1.095, 1.1575, 1.2051};
double fps_table[16] = {0.0, 23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0};
char frame_type_table[8] = { 0, 'I', 'P', 'B', 'D' };
int block_comp[6] = { 0, 0, 0, 0, 1, 2 };

void decodeHeader(MPEG1Data &mpg)
{
    mpg.width = mpg.stream.nextbits(12);
    mpg.height = mpg.stream.nextbits(12);
    mpg.width_mb = (mpg.width - 1) / 16 + 1;
    mpg.height_mb = (mpg.height - 1) / 16 + 1;
    printf("size: %dx%d\n", mpg.width, mpg.height);
    mpg.pixel_ar = pixel_ar_table[mpg.stream.nextbits(4)];
    mpg.fps = fps_table[mpg.stream.nextbits(4)];
    printf("pixel AR: %f\n", mpg.pixel_ar);
    printf("fps: %f\n", mpg.fps);
    mpg.stream.nextbits(30); // bitrate ignored
    if (mpg.stream.nextbits(1)) {
        printf("Load intra Q matrix\n");
        for (int i = 0; i < 64; i++) mpg.q_intra[i] = mpg.stream.nextbits(8);
    }
    if (mpg.stream.nextbits(1)) {
        printf("Load non-intra Q matrix\n");
        for (int i = 0; i < 64; i++) mpg.q_nonintra[i] = mpg.stream.nextbits(8);
    }
    mpg.next_start_code = mpg.stream.nextbits(32);
}

inline void decodeBlock(MPEG1Data &mpg, int id)
{
    int block_zz[64];
    std::fill_n(block_zz, 64, 0);
    if (mpg.cur_mb.intra) {
        int size, diff = 0;
        if (id < 4) size = Tables::dct_dc_luma.get();
        else size = Tables::dct_dc_chroma.get();
        if (size) {
            diff = mpg.stream.nextbits(size);
            if (diff < 1 << (size - 1)) diff -= (1 << size) - 1;
        }
        block_zz[0] = mpg.cur_slice.dc_predictor[block_comp[id]] + diff;
    }
    else{
        // TODO: P, B
    }
    mpg.cur_slice.dc_predictor[block_comp[id]] = block_zz[0];
    int run_level, pos = 0;
    while ((run_level = Tables::dct_coeff_next.get()) != -1) { // EOB
        int run, level;
        if (run_level == -2) {
            run = mpg.stream.nextbits(6);
            level = (char)mpg.stream.nextbits(8);
            if (level == 0 || level == 0xffffff80) level = level << 1 | mpg.stream.nextbits(8);
        }
        else{
            run = run_level >> 8;
            level = (char)run_level;
        }
        pos += run + 1;
        block_zz[pos] = level;
    }
}

inline void decodeMacroblock(MPEG1Data &mpg)
{
    int tmp;
    int n_escape = 0;
    do {
        tmp = Tables::macro_addrinc.get();
        if (tmp == -2) n_escape++; // escape
        // ignore stuffing
    } while (tmp < 0);
    mpg.cur_mb.addr = mpg.cur_slice.last_mb_addr + tmp + n_escape * 33;
    mpg.cur_slice.last_mb_addr = mpg.cur_mb.addr;
    printf("Macroblock pos=(%d, %d)\n", mpg.cur_mb.addr / mpg.width_mb, mpg.cur_mb.addr % mpg.width_mb);
    switch (mpg.cur_picture.type) {
    case 'I': tmp = Tables::macro_I.get(); break;
    case 'P': tmp = Tables::macro_P.get(); break;
    case 'B': tmp = Tables::macro_B.get(); break;
    default: throw "picture type not supported";
    }
    mpg.cur_mb.quant = tmp & 1 << 4;
    mpg.cur_mb.motion_forward = tmp & 1 << 3;
    mpg.cur_mb.motion_backward = tmp & 1 << 2;
    mpg.cur_mb.pattern = tmp & 1 << 1;
    mpg.cur_mb.intra = tmp & 1;
    mpg.cur_mb.q_scale = mpg.cur_mb.quant ? mpg.stream.nextbits(5) : mpg.cur_slice.q_scale;
    if (mpg.cur_mb.motion_forward) {
        // TODO: P, B
    }
    if (mpg.cur_mb.motion_backward) {
        // TODO: B
    }
    int pattern = 0x3f;
    if (mpg.cur_mb.pattern) pattern = Tables::cbp.get();
    for (int i = 0; i < 6; i++)
        if (pattern & 1 << (5 - i)) decodeBlock(mpg, i);
}

inline void decodeSlice(MPEG1Data &mpg)
{
    mpg.cur_slice.vpos = mpg.next_start_code & 0xff;
    mpg.cur_slice.q_scale = mpg.stream.nextbits(5);
    mpg.cur_slice.last_mb_addr = (mpg.cur_slice.vpos - 1) * mpg.width_mb - 1;
    mpg.cur_slice.dc_predictor[0] = mpg.cur_slice.dc_predictor[1] = mpg.cur_slice.dc_predictor[2] = 1024;
    while (mpg.stream.nextbits(1)) {
        // extra info ignored
        mpg.stream.nextbits(8);
    }
    while (mpg.stream.nextbits(23, false) != 0) {
        decodeMacroblock(mpg);
    }
    mpg.stream.align();
    mpg.next_start_code = mpg.stream.nextbits(32);
}

inline void decodePicture(MPEG1Data &mpg)
{
    int temp_ref = mpg.stream.nextbits(10);
    char frame_type = frame_type_table[mpg.stream.nextbits(3)];
    mpg.stream.nextbits(16); // VBV delay ignored
    printf("Picture #%d: %c", temp_ref, frame_type);
    mpg.cur_picture.temp_ref = temp_ref;
    mpg.cur_picture.type = frame_type;
    if (frame_type == 'P' || frame_type == 'B') {
        mpg.cur_picture.full_pel_forward = mpg.stream.nextbits(1);
        int forward_f_code = mpg.stream.nextbits(3);
        mpg.cur_picture.forward_f = 1 << (forward_f_code - 1);
        mpg.cur_picture.forward_f_size = forward_f_code - 1;
        if (frame_type == 'B') {
            mpg.cur_picture.full_pel_backward = mpg.stream.nextbits(1);
            int backward_f_code = mpg.stream.nextbits(3);
            mpg.cur_picture.backward_f = 1 << (backward_f_code - 1);
            mpg.cur_picture.backward_f_size = backward_f_code - 1;
        }
    }
    printf("\n");
    mpg.stream.align();
    mpg.next_start_code = mpg.stream.nextbits(32);
    while (mpg.next_start_code >= 0x00000101 && mpg.next_start_code <= 0x000001af) {
        decodeSlice(mpg);
    }
}

void decodeGOP(MPEG1Data &mpg)
{
    int hours = mpg.stream.nextbits(6) & 0x1f,
        minutes = mpg.stream.nextbits(6),
        seconds = mpg.stream.nextbits(7) & 0x2f,
        picture = mpg.stream.nextbits(6);
    bool closed_gop = mpg.stream.nextbits(1), broken_link = mpg.stream.nextbits(1);
    printf("GOP timecode: %02d:%02d:%02d:%02d\n", hours, minutes, seconds, picture);
    mpg.stream.align();
    mpg.next_start_code = mpg.stream.nextbits(32);
    while (mpg.next_start_code == 0x00000100) {
        decodePicture(mpg);
    }
}