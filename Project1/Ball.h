#ifndef BALL_H
#define BALL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"

struct Edge {
    glm::vec3 start;
    glm::vec3 end;
    glm::vec3 normal;
    float cushionWidth;

    Edge(glm::vec3 s, glm::vec3 e, glm::vec3 n, float w)
        : start(s), end(e), normal(n), cushionWidth(w) {}
};

struct Ball {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 color;

    float radius;
    float mass;
    float restitution;
    float friction;

    int number;
    bool pocketed;

    GLuint VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    std::unique_ptr<Shader> shader;

    Ball(float x, float y, float z, float r, glm::vec3 col, int num);

    void checkHolePocketed();
    void update(float deltaTime);
    bool checkCollision(Ball* other);
    void resolveCollision(Ball* other);
    bool checkEdgeCollision(const Edge& edge) const;
    void resolveEdgeCollision(const Edge& edge);
    void generateSphere();
    void setupBuffers();
    void cleanup();
};

#endif