#ifndef CUE_H
#define CUE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

#include "Constants.h"

struct Cue {
    glm::vec3 position;

    float length;
    float thickness;
    float shotPower;

    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    Cue(float len, float thick);
    void setShotPower(float power);
    void updateCuePosition();
    void generateCue();
    void updateGeometry();
    void setupBuffers();
    void cleanup();
};

#endif