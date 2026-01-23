/**
 * cube.cpp, by electro707
 *
 * This was ported from one of my projects, thus why it's mostly c-like with little classes and a self-made matrix
 *  multiplication math
 */
#include "display/cube.h"
#include "display/DisplayManager.h"
#include "config/ConfigManager.h"
#include <Arduino_GFX_Library.h>

#define CUBE_MATRIX_ROWS 10

// the cube matrix,
float cubePositions[CUBE_MATRIX_ROWS * 4] = {
    -1, 1, -1, 1, 1, 1, -1, 1, 1, -1, -1, 1, -1, -1, -1, 1, -1, 1, -1, 1,
    -1, 1, 1,  1, 1, 1, 1,  1, 1, -1, 1,  1, -1, -1, 1,  1, -1, 1, 1,  1,
};

typedef struct {
    u32 rows;
    u32 cols;
} matrixSize_s;

extern Arduino_GFX* g_lcd;
extern ConfigManager configManager;

// self-made matrix multiplication
void matrixMult(float* srcA, float* srcB, float* dst, matrixSize_s sizeA, matrixSize_s sizeB) {
    float sum;

    assert(sizeA.cols == sizeB.rows);

    u32 m = sizeA.rows;
    u32 p = sizeB.cols;
    u32 n = sizeA.cols;

    memset(dst, 0, m * p * sizeof(float));

    for (u32 i = 0; i < m; i++) {
        for (u32 j = 0; j < p; j++) {
            sum = 0;
            for (u32 k = 0; k < n; k++) {
                sum += srcA[k + (i * sizeA.cols)] * srcB[j + (k * sizeB.cols)];
            }
            dst[(i * sizeB.cols) + j] = sum;
        }
    }
}

void rotateCube(float rotX, float rotY, float rotZ, float* newCube) {
    float tmpA[4 * CUBE_MATRIX_ROWS];
    float tmpB[4 * CUBE_MATRIX_ROWS];

    rotX = rotX * (M_PI / 180);
    rotY = rotY * (M_PI / 180);
    rotZ = rotZ * (M_PI / 180);

    float rotationMatrixX[4 * 4] = {
        1, 0, 0, 0, 0, cos(rotX), -sin(rotX), 0, 0, sin(rotX), cos(rotX), 0, 0, 0, 0, 1,
    };
    float rotationMatrixY[4 * 4] = {
        cos(rotY), 0, sin(rotY), 0, 0, 1, 0, 0, -sin(rotY), 0, cos(rotY), 0, 0, 0, 0, 1,
    };
    float rotationMatrixZ[4 * 4] = {
        cos(rotZ), -sin(rotZ), 0, 0, sin(rotZ), cos(rotZ), 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    };

    matrixMult(cubePositions, rotationMatrixX, tmpA, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
    matrixMult(tmpA, rotationMatrixY, tmpB, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
    matrixMult(tmpB, rotationMatrixZ, newCube, (matrixSize_s){CUBE_MATRIX_ROWS, 4}, (matrixSize_s){4, 4});
}

/**
 * @brief This function takes a float position and converts it to on-screen coordinates
 *
 * Because the screen for this firmware will always be 240x240, this works
 *
 * todo: have this use globally defined LCD, but NOT through the config (no need, static display size)
 */
u32 scaleVectorToDraw(float pos) {
    return (pos * (configManager.getLCDWidthSafe() / 4)) + (configManager.getLCDWidthSafe() / 2);
}

Cube::Cube() {}

auto Cube::setRotX(float newRot) -> void { rotX = newRot; }
auto Cube::setRotY(float newRot) -> void { rotY = newRot; }

auto Cube::draw(void) -> void {
    float* pos;
    float* posNext;
    float cube[CUBE_MATRIX_ROWS * 4];

    rotateCube(rotX, rotY, rotZ, cube);

    g_lcd->startWrite();

    // draw the top and bottom squares
    for (int side = 0; side < 2; side++) {
        for (int i = 0; i < 4; i++) {
            pos = &cube[(4 * i) + (side * 5 * 4)];
            posNext = &cube[4 * (i + 1) + (side * 5 * 4)];

            g_lcd->writeLine(scaleVectorToDraw(pos[0]), scaleVectorToDraw(pos[1]), scaleVectorToDraw(posNext[0]),
                             scaleVectorToDraw(posNext[1]), LCD_WHITE);
        }
    }

    // draw the 4 lines connecting them
    for (int p = 0; p < 4; p++) {
        pos = &cube[4 * p];
        posNext = &cube[4 * (p + 5)];

        g_lcd->writeLine(scaleVectorToDraw(pos[0]), scaleVectorToDraw(pos[1]), scaleVectorToDraw(posNext[0]),
                         scaleVectorToDraw(posNext[1]), LCD_WHITE);
    }

    g_lcd->endWrite();
}