#ifndef BUTTON_H
#define BUTTON_H

#include <string>

struct Button {
    std::string text;
    float x, y;
    float width, height;

    bool isHovered(float mouseX, float mouseY) {
        return mouseX >= x && mouseX <= x + width &&
            mouseY >= y && mouseY <= y + height;
    }
};

#endif