#include "MPEG1Decoder.h"

double pixel_ar_table[16] = {0.0, 1.0, 0.6735, 0.7031, 0.7615, 0.8055, 0.8437, 0.8935, 0.9157, 0.9815, 1.0255, 1.0695, 1.095, 1.1575, 1.2051};
double fps_table[16] = {0.0, 23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0};
char frame_type_table[8] = { 0, 'I', 'P', 'B', 'D' };

void decodeHeader(MPEG1Data &mpg)
{
    mpg.width = mpg.stream.nextbits(12);
    mpg.height = mpg.stream.nextbits(12);
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
    else {
        for (int i = 0; i < 64; i++) mpg.q_intra[i] = 1;
    }
    if (mpg.stream.nextbits(1)) {
        printf("Load non-intra Q matrix\n");
        for (int i = 0; i < 64; i++) mpg.q_nonintra[i] = mpg.stream.nextbits(8);
    }
    else {
        for (int i = 0; i < 64; i++) mpg.q_nonintra[i] = 1;
    }
    mpg.next_start_code = mpg.stream.nextbits(32);
}

inline void decodeMacroblock(MPEG1Data &mpg)
{
    mpg.stream.nextbits(1);
}

inline void decodeSlice(MPEG1Data &mpg)
{
    mpg.cur_slice.vpos = mpg.next_start_code & 0xff;
    mpg.cur_slice.q_scale = mpg.stream.nextbits(5);
    while (mpg.stream.nextbits(23, false) != 0) {
        decodeMacroblock(mpg);
    }
    mpg.stream.align();
    // XXX: remove redundant bit for test
    if (mpg.stream.nextbits(32, false) == 0x00000001)
        mpg.stream.nextbits(8);
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