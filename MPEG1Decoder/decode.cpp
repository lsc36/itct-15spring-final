#include "MPEG1Decoder.h"

double pixel_ar_table[16] = {0.0, 1.0, 0.6735, 0.7031, 0.7615, 0.8055, 0.8437, 0.8935, 0.9157, 0.9815, 1.0255, 1.0695, 1.095, 1.1575, 1.2051};
double fps_table[16] = {0.0, 23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0};

void decodeHeader(MPEG1Data &mpg)
{
    uint8_t buf[256];
    fread(buf, 1, 3, mpg.fp);
    mpg.width = buf[0] << 4 | buf[1] >> 4;
    mpg.height = (buf[1] & 0xf) << 8 | buf[2];
    printf("size: %dx%d\n", mpg.width, mpg.height);
    fread(buf, 1, 1, mpg.fp);
    mpg.pixel_ar = pixel_ar_table[buf[0] >> 4];
    mpg.fps = fps_table[buf[0] & 0xf];
    printf("pixel AR: %f\n", mpg.pixel_ar);
    printf("fps: %f\n", mpg.fps);
    fread(buf, 1, 4, mpg.fp);
    // bitrate ignored
    if (buf[3] & 0x2) {
        printf("Load intra Q matrix");
        fread(&buf[4], 1, 64, mpg.fp);
        for (int i = 0; i < 64; i++) mpg.q_intra[i] = buf[i + 3] << 7 | buf[i + 4] >> 1;
        buf[3] = buf[67];
    }
    else {
        for (int i = 0; i < 64; i++) mpg.q_intra[i] = 1;
    }
    if (buf[3] & 0x1) {
        printf("Load non-intra Q matrix");
        fread(&mpg.q_nonintra, 1, 64, mpg.fp);
    }
    else {
        for (int i = 0; i < 64; i++) mpg.q_nonintra[i] = 1;
    }
    fread(&mpg.next_start_code, 4, 1, mpg.fp);
}

inline void decodePicture(MPEG1Data &mpg)
{
    fread(&mpg.next_start_code, 4, 1, mpg.fp);
}

void decodeGOP(MPEG1Data &mpg)
{
    uint8_t buf[4];
    fread(buf, 1, 4, mpg.fp);
    int hours = (buf[0] >> 2) & 0x1f,
        minutes = (buf[0] & 0x3) << 4 | buf[1] >> 4,
        seconds = (buf[1] & 0x7) << 3 | buf[2] >> 5,
        picture = (buf[2] & 0x1f) << 1 | buf[3] >> 7;
    bool closed_gop = buf[3] & 0x40, broken_link = buf[3] & 0x20;
    printf("GOP timecode: %02d:%02d:%02d:%02d\n", hours, minutes, seconds, picture);
    fread(&mpg.next_start_code, 4, 1, mpg.fp);
    while (mpg.next_start_code == 0x00010000) {
        decodePicture(mpg);
    }
}