#ifndef SRC_DISPLAY_CUBE_H
#define SRC_DISPLAY_CUBE_H

#include <Arduino.h>

class Cube {
   public:
    Cube();
    void draw();
    void setRotX(float newRot);
    void setRotY(float newRot);

    // current cube rotation
    float rotX;
    float rotY;
    float rotZ;
};

#endif