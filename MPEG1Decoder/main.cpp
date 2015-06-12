#include "MPEG1Decoder.h"
#include <GL\glut.h>

MPEG1Data mpg;
Pixel *currentFrame;

void decodeThread()
{
    bool finished = false;
    while (true) {
        if (finished) {
            Sleep(1);
            continue;
        }
        uint32_t start_code;
        fread(&start_code, sizeof(uint32_t), 1, mpg.fp);
        switch (start_code) {
        case 0xb8010000:
            decodeGOP(mpg);
            break;
        case 0xb7010000:
            finished = true;
            break;
        default:
            fprintf(stderr, "unknown start code %08x\n", start_code);
            exit(1);
        }
    }
}

void fetchFrame(int value)
{
    if (mpg.frames.try_pop(currentFrame)) glutPostRedisplay();
    glutTimerFunc(1000 / mpg.fps, fetchFrame, 0);
}

void display()
{
    if (currentFrame != NULL) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawPixels(mpg.width, mpg.height, GL_RGB, GL_UNSIGNED_BYTE, currentFrame);
        glutSwapBuffers();
        delete currentFrame;
        currentFrame = NULL;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    }

    mpg.fp = fopen(argv[1], "rb");
    assert(mpg.fp != NULL);
    decodeHeader(mpg);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(mpg.width, mpg.height);
    glutCreateWindow("MPEG1Decoder");

    glutDisplayFunc(display);
    glutTimerFunc(1000 / mpg.fps, fetchFrame, 0);

    std::thread t_decode(decodeThread);
    glutMainLoop();

    return 0;
}