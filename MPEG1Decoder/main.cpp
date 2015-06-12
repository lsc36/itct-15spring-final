#include "MPEG1Decoder.h"
#include <GL\glut.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    }

    MPEG1Data mpg;
    mpg.fp = fopen(argv[1], "rb");
    assert(mpg.fp != NULL);
    decodeHeader(mpg);

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(mpg.width, mpg.height);
    glutCreateWindow("MPEG1Decoder");

    glutMainLoop();

    return 0;
}