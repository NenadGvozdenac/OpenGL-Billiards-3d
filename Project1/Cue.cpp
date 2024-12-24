#include "Cue.h"

Cue::Cue(float len = 2.0f, float thick = 0.02f)
    : length(len), thickness(thick), shotPower(2.0f) {
    position = glm::vec3(0.0f);
    generateCue();
    setupBuffers();
}

void Cue::setShotPower(float power) {
    shotPower = glm::clamp(power, 2.0f, 10.f);
    updateCuePosition();
}

void Cue::updateCuePosition() {
    // Linear interpolation between minimum (2.0) and maximum (10.0) power
    // As power increases, cue moves back further
    float powerRatio = (shotPower - 2.0f) / (10.f - 2.0f);
    float backOffset = powerRatio * 0.5f; // Maximum 0.5 units back from starting position

    position.z = -backOffset;
}

void Cue::generateCue() {
    const int segments = 8;
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * float(i) / float(segments);
        float x = cos(angle) * thickness;
        float y = sin(angle) * thickness;

        // Front vertices (tip of cue)
        vertices.push_back(x + position.x);
        vertices.push_back(y + position.y);
        vertices.push_back(0.0f + position.z);
        // Normal
        vertices.push_back(x / thickness);
        vertices.push_back(y / thickness);
        vertices.push_back(0.0f);

        // Back vertices
        vertices.push_back(x + position.x);
        vertices.push_back(y + position.y);
        vertices.push_back(-length + position.z);
        // Normal
        vertices.push_back(x / thickness);
        vertices.push_back(y / thickness);
        vertices.push_back(0.0f);
    }

    for (int i = 0; i < segments; i++) {
        unsigned int current = i * 2;
        unsigned int next = (i + 1) * 2;
        // First triangle
        indices.push_back(current);
        indices.push_back(next);
        indices.push_back(current + 1);
        // Second triangle
        indices.push_back(current + 1);
        indices.push_back(next);
        indices.push_back(next + 1);
    }
}

void Cue::updateGeometry() {
    vertices.clear();
    indices.clear();
    generateCue();

    // Update buffer data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
}

void Cue::setupBuffers() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Cue::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}