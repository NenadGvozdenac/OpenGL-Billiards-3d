#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;

    int currentView;
    int currentZoom;

    Camera();
    void setView(int viewNumber);
    void setZoom(int zoomNumber);
    glm::mat4 getViewMatrix();
};

#endif