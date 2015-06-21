#include "MPEG1Decoder.h"

double pixel_ar_table[16] = {0.0, 1.0, 0.6735, 0.7031, 0.7615, 0.8055, 0.8437, 0.8935, 0.9157, 0.9815, 1.0255, 1.0695, 1.095, 1.1575, 1.2051};
double fps_table[16] = {0.0, 23.976, 24.0, 25.0, 29.97, 30.0, 50.0, 59.94, 60.0};
char frame_type_table[8] = { 0, 'I', 'P', 'B', 'D' };
int block_comp[6] = { 0, 0, 0, 0, 1, 2 };
int scan[64] = { 0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25, 30, 41, 43, 9, 11, 18, 24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63 };

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
    int run_level, run, level, pos = 0;
    if (mpg.cur_mb.intra) {
        int size, diff = 0;
        if (id < 4) size = Tables::dct_dc_luma.get();
        else size = Tables::dct_dc_chroma.get();
        if (size) {
            diff = mpg.stream.nextbits(size);
            if (diff < 1 << (size - 1)) diff -= (1 << size) - 1;
        }
        block_zz[0] = mpg.cur_slice.dc_predictor[block_comp[id]] + 8 * diff;
        mpg.cur_slice.dc_predictor[block_comp[id]] = block_zz[0];
    }
    else{
        run_level = Tables::dct_coeff_first.get();
        if (run_level == -2) {
            run = mpg.stream.nextbits(6);
            level = (char)mpg.stream.nextbits(8);
            if (level == 0 || level == 0xffffff80) level = level << 1 | mpg.stream.nextbits(8);
        }
        else{
            run = run_level >> 8;
            level = (char)run_level;
        }
        pos = run;
        block_zz[pos] = level;
    }
    while ((run_level = Tables::dct_coeff_next.get()) != -1) { // EOB
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
    if (mpg.cur_mb.intra) {
        for (int i = 1; i < 64; i++) {
            block_zz[i] = (2 * block_zz[i] * mpg.cur_mb.q_scale * mpg.q_intra[i]) / 16;
            if (!block_zz & 1) block_zz[i] -= sign(block_zz[i]);
            block_zz[i] = clip(block_zz[i], -2048, 2047);
        }
    }
    else {
        for (int i = 0; i < 64; i++) {
            block_zz[i] = ((2 * block_zz[i]) + sign(block_zz[i]) * mpg.cur_mb.q_scale * mpg.q_nonintra[i]) / 16;
            if (!block_zz & 1) block_zz[i] -= sign(block_zz[i]);
            block_zz[i] = clip(block_zz[i], -2048, 2047);
         }
    }
    for (int i = 0; i < 64; i++) mpg.cur_mb.block[id][i] = block_zz[scan[i]];
    idct(mpg.cur_mb.block[id]);
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
    //printf("Macroblock pos=(%d, %d)\n", mpg.cur_mb.addr / mpg.width_mb, mpg.cur_mb.addr % mpg.width_mb);
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
    mpg.cur_slice.q_scale = mpg.cur_mb.q_scale;
    if (!mpg.cur_mb.intra || mpg.cur_mb.addr - mpg.cur_slice.last_intra_addr > 1) {
        mpg.cur_slice.dc_predictor[0] = mpg.cur_slice.dc_predictor[1] = mpg.cur_slice.dc_predictor[2] = 1024;
    }
    if (mpg.cur_mb.intra) mpg.cur_slice.last_intra_addr = mpg.cur_mb.addr;
    // motion vectors
    if (mpg.cur_mb.addr - mpg.cur_slice.last_mb_addr > 1) {
        mpg.cur_slice.recon_right_for_prev = mpg.cur_slice.recon_down_for_prev
            = mpg.cur_slice.recon_right_back_prev = mpg.cur_slice.recon_down_back_prev = 0;
    }
    int base_x = (mpg.cur_mb.addr / mpg.width_mb) * 16,
        base_y = (mpg.cur_mb.addr % mpg.width_mb) * 16;
    int right_little, right_big, down_little, down_big,
        recon_right_for, recon_down_for, mmax, mmin;
    if (mpg.cur_mb.motion_forward) {
        int forward_h_code, comp_forward_h_r = 0,
            forward_v_code, comp_forward_v_r = 0;
        forward_h_code = Tables::mv.get();
        if (mpg.cur_picture.forward_f != 1 && forward_h_code != 0) {
            tmp = mpg.stream.nextbits(mpg.cur_picture.forward_r_size); // motion_horizontal_forward_r
            comp_forward_h_r = mpg.cur_picture.forward_f - 1 - tmp;
        }
        forward_v_code = Tables::mv.get();
        if (mpg.cur_picture.forward_f != 1 && forward_v_code != 0) {
            tmp = mpg.stream.nextbits(mpg.cur_picture.forward_r_size); // motion_vertical_forward_r
            comp_forward_v_r = mpg.cur_picture.forward_f - 1 - tmp;
        }
        // start video 2.4.4.2
        right_little = forward_h_code * mpg.cur_picture.forward_f;
        if (right_little > 0) {
            right_little -= comp_forward_h_r;
            right_big = right_little - 32 * mpg.cur_picture.forward_f;
        }
        else if (right_little < 0) {
            right_little += comp_forward_h_r;
            right_big = right_little + 32 * mpg.cur_picture.forward_f;
        }
        else right_big = 0;
        down_little = forward_v_code * mpg.cur_picture.forward_f;
        if (down_little > 0) {
            down_little -= comp_forward_v_r;
            down_big = down_little - 32 * mpg.cur_picture.forward_f;
        }
        else if (down_little < 0) {
            down_little += comp_forward_v_r;
            down_big = down_little + 32 * mpg.cur_picture.forward_f;
        }
        else down_big = 0;
        mmax = (mpg.cur_picture.forward_f << 4) - 1;
        mmin = -(mpg.cur_picture.forward_f << 4);
        tmp = mpg.cur_slice.recon_right_for_prev + right_little;
        if (mmin <= tmp && tmp <= mmax) recon_right_for = tmp;
        else recon_right_for = tmp - right_little + right_big;
        mpg.cur_slice.recon_right_for_prev = recon_right_for;
        if (mpg.cur_picture.full_pel_forward) recon_right_for <<= 1;
        tmp = mpg.cur_slice.recon_down_for_prev + down_little;
        if (mmin <= tmp && tmp <= mmax) recon_down_for = tmp;
        else recon_down_for = tmp - down_little + down_big;
        mpg.cur_slice.recon_down_for_prev = recon_down_for;
        if (mpg.cur_picture.full_pel_forward) recon_down_for <<= 1;
        // copy pixels
        for (int i = 0; i < 16 && base_x + i < mpg.height; i++) {
            for (int j = 0; j < 16 && base_y + j < mpg.width; j++) {
                int x = mpg.height - (base_x + i) - 1,
                    y = base_y + j;
                int forx = mpg.height - (base_x + i + recon_down_for / 2) - 1,
                    fory = base_y + j + recon_right_for / 2;
                if (0 <= forx && forx < mpg.height && 0 <= fory && fory < mpg.width)
                    mpg.cur_picture.buffer[x * mpg.width + y] = mpg.forward_ref[forx * mpg.width + fory];
                else
                    mpg.cur_picture.buffer[x * mpg.width + y] = Pixel(0, 0, 0);
            }
        }
    }
    else {
        mpg.cur_slice.recon_right_for_prev = mpg.cur_slice.recon_down_for_prev = 0;
    }
    int backward_h_code, backward_h_r,
        backward_v_code, backward_v_r;
    if (mpg.cur_mb.motion_backward) {
        backward_h_code = Tables::mv.get();
        if (mpg.cur_picture.backward_f != 1 && backward_h_code != 0) {
            backward_h_r = mpg.stream.nextbits(mpg.cur_picture.backward_r_size);
        }
        backward_v_code = Tables::mv.get();
        if (mpg.cur_picture.backward_f != 1 && backward_v_code != 0) {
            backward_v_r = mpg.stream.nextbits(mpg.cur_picture.backward_r_size);
        }
        // TODO
    }
    else {
        mpg.cur_slice.recon_right_back_prev = mpg.cur_slice.recon_down_back_prev = 0;
    }
    // coded block pattern
    int pattern = mpg.cur_mb.intra ? 0x3f : 0;
    if (mpg.cur_mb.pattern) pattern = Tables::cbp.get();
    for (int i = 0; i < 6; i++)
        if (pattern & 1 << (5 - i)) decodeBlock(mpg, i);
    for (int i = 0; i < 16 && base_x + i < mpg.height; i++) {
        for (int j = 0; j < 16 && base_y + j < mpg.width; j++) {
            int Y_id = 0;
            if (i >= 8 && j >= 8) Y_id = 3;
            else if (i >= 8) Y_id = 2;
            else if (j >= 8) Y_id = 1;
            int Y = mpg.cur_mb.block[Y_id][(i % 8) * 8 + j % 8],
                Cb = mpg.cur_mb.block[4][(i / 2) * 8 + (j / 2)],
                Cr = mpg.cur_mb.block[5][(i / 2) * 8 + (j / 2)];
            int x = mpg.height - (base_x + i) - 1,
                y = base_y + j;
            if (mpg.cur_mb.intra) {
                int R = clip((int)round(Y + 1.402 * (Cr - 128.0)), 0, 255),
                    G = clip((int)round(Y - 0.34414 * (Cb - 128.0) - 0.71414 * (Cr - 128.0)), 0, 255),
                    B = clip((int)round(Y + 1.772 * (Cb - 128.0)), 0, 255);
                mpg.cur_picture.buffer[x * mpg.width + y] = Pixel(R, G, B);
            }
            else {
                if (!(pattern & 1 << (5 - Y_id))) Y = 0;
                if (!(pattern & 1 << 1)) Cb = 0;
                if (!(pattern & 1 << 0)) Cr = 0;
                mpg.cur_picture.buffer[x * mpg.width + y].diff(
                    round(Y + 1.402 * Cr),
                    round(Y - 0.34414 * Cb - 0.71414 * Cr),
                    round(Y + 1.772 * Cb)
                    );
            }
        }
    }
    mpg.cur_slice.last_mb_addr = mpg.cur_mb.addr;
}

inline void decodeSlice(MPEG1Data &mpg)
{
    mpg.cur_slice.vpos = mpg.next_start_code & 0xff;
    mpg.cur_slice.q_scale = mpg.stream.nextbits(5);
    mpg.cur_slice.last_mb_addr = (mpg.cur_slice.vpos - 1) * mpg.width_mb - 1;
    mpg.cur_slice.last_intra_addr = -2;
    mpg.cur_slice.recon_right_for_prev = mpg.cur_slice.recon_down_for_prev
        = mpg.cur_slice.recon_right_back_prev = mpg.cur_slice.recon_down_back_prev = 0;
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
        mpg.cur_picture.forward_r_size = forward_f_code - 1;
        if (frame_type == 'B') {
            mpg.cur_picture.full_pel_backward = mpg.stream.nextbits(1);
            int backward_f_code = mpg.stream.nextbits(3);
            mpg.cur_picture.backward_f = 1 << (backward_f_code - 1);
            mpg.cur_picture.backward_r_size = backward_f_code - 1;
        }
    }
    printf("\n");
    if (frame_type != 'B') {
        if (mpg.forward_ref != NULL) delete[] mpg.forward_ref;
        mpg.forward_ref = mpg.backward_ref;
        mpg.backward_ref = NULL;
    }
    mpg.cur_picture.buffer = new Pixel[mpg.width * mpg.height];
    if (mpg.forward_ref != NULL) std::copy_n(mpg.forward_ref, mpg.width * mpg.height, mpg.cur_picture.buffer);
    mpg.stream.align();
    mpg.next_start_code = mpg.stream.nextbits(32);
    while (mpg.next_start_code >= 0x00000101 && mpg.next_start_code <= 0x000001af) {
        decodeSlice(mpg);
    }
    Pixel *out = NULL;
    if (frame_type == 'B') {
        out = mpg.cur_picture.buffer;
    }
    else {
        mpg.backward_ref = mpg.cur_picture.buffer;
        if (mpg.forward_ref != NULL) {
            out = new Pixel[mpg.width * mpg.height];
            std::copy_n(mpg.forward_ref, mpg.width * mpg.height, out);
        }
    }
    if (out != NULL) {
        mpg.mtx_frames.lock();
        while (mpg.frames.size() > 2 * mpg.fps) {
            mpg.mtx_frames.unlock();
            printf("decoding too fast, sleep...\n");
            Sleep(1000);
            mpg.mtx_frames.lock();
        }
        mpg.frames.push(out);
        mpg.mtx_frames.unlock();
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