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
        switch (mpg.next_start_code) {
        case 0x000001b3:
            decodeHeader(mpg);
            break;
        case 0x000001b8:
            decodeGOP(mpg);
            break;
        case 0x000001b7:
            printf("decode finished\n");
            finished = true;
            break;
        default:
            fprintf(stderr, "unknown start code %08x\n", mpg.next_start_code);
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
        return 1;
    }

    try {
        mpg.stream.open(argv[1]);
    }
    catch (const char *msg){
        fprintf(stderr, "Error: %s\n", msg);
        return 1;
    }
    if (mpg.stream.nextbits(32) != 0x000001b3) {
        fprintf(stderr, "Error: not an MPEG1 sequence\n");
        return 1;
    }
    decodeHeader(mpg);
    Tables::initTables(mpg.stream);

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