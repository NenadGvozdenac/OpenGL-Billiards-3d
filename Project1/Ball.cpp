#include "Ball.h"
#include "Constants.h"


Ball::Ball(float x, float y, float z, float r, glm::vec3 col, int num)
    : position(x, y, z),
    velocity(0.0f, 0.0f, 0.0f),
    radius(r),
    color(col),
    number(num),
    mass(1.5f),
    restitution(0.8f),
    friction(0.25f),
    pocketed(false) {
    generateSphere();
    setupBuffers();
}

// Add this to your Ball class
void Ball::checkHolePocketed() {
    // Hole positions (corners and middle pockets)
    const std::vector<glm::vec2> holePositions = {
        glm::vec2(-2.0f, 1.0f),    // Top-left corner
        glm::vec2(0.0f, 1.0f),     // Top-middle
        glm::vec2(2.0f, 1.0f),     // Top-right corner
        glm::vec2(-2.0f, -1.0f),   // Bottom-left corner
        glm::vec2(0.0f, -1.0f),    // Bottom-middle
        glm::vec2(2.0f, -1.0f)     // Bottom-right corner
    };

    const float holeRadius = 0.15f;  // Same as hole radius in setupHoles()

    // Check distance to each hole
    for (const auto& holePos : holePositions) {
        float distance = glm::length(glm::vec2(position.x, position.z) - holePos);
        if (distance < holeRadius) {
            pocketed = true;
            // Move the ball below the table to hide it
            position.y = -1.0f;
            velocity = glm::vec3(0.0f);
            break;
        }
    }
}

void Ball::update(float deltaTime) {
    // Update position based on velocity
    position += velocity * deltaTime;

    // Apply friction to slow the ball
    if (glm::length(velocity) > 0.0f) {
        glm::vec3 frictionForce = -glm::normalize(velocity) * friction;
        velocity += frictionForce * deltaTime;

        // Stop the ball if velocity is very small
        if (glm::length(velocity) < 0.01f) {
            velocity = glm::vec3(0.0f);
        }
    }
}

bool Ball::checkCollision(Ball* other) {
    if (pocketed || other->pocketed) return false;
    float distance = glm::length(position - other->position);
    return distance < (radius + other->radius);
}

void Ball::resolveCollision(Ball* other) {
    glm::vec3 normal = glm::normalize(other->position - position);
    glm::vec3 relativeVelocity = other->velocity - velocity;

    float velocityAlongNormal = glm::dot(relativeVelocity, normal);

    // Only resolve if balls are moving toward each other
    if (velocityAlongNormal > 0) return;

    float combinedRestitution = (restitution + other->restitution) * 0.5f;
    float j = -(1.0f + combinedRestitution) * velocityAlongNormal;
    j /= 1.0f / mass + 1.0f / other->mass;

    glm::vec3 impulse = j * normal;
    velocity -= impulse / mass;
    other->velocity += impulse / other->mass;

    // Separate balls to prevent sticking
    float overlap = radius + other->radius - glm::length(other->position - position);
    if (overlap > 0) {
        glm::vec3 separation = normal * overlap * 0.5f;
        position -= separation;
        other->position += separation;
    }
}

bool Ball::checkEdgeCollision(const Edge& edge) const {
    if (pocketed) return false;

    // Vector from edge start to ball center
    glm::vec3 ballToEdgeStart = position - edge.start;

    // Edge vector
    glm::vec3 edgeVector = edge.end - edge.start;
    float edgeLength = glm::length(edgeVector);
    glm::vec3 edgeDirection = edgeVector / edgeLength;

    // Project ball position onto edge
    float t = glm::dot(ballToEdgeStart, edgeDirection);

    // Find closest point on edge to ball
    glm::vec3 closestPoint;
    if (t < 0) closestPoint = edge.start;
    else if (t > edgeLength) closestPoint = edge.end;
    else closestPoint = edge.start + edgeDirection * t;

    // Check distance from ball to closest point
    float distance = glm::length(position - closestPoint);
    return distance < (radius + edge.cushionWidth);
}

void Ball::resolveEdgeCollision(const Edge& edge) {
    // Find closest point on edge similar to collision check
    glm::vec3 ballToEdgeStart = position - edge.start;
    glm::vec3 edgeVector = edge.end - edge.start;
    float edgeLength = glm::length(edgeVector);
    glm::vec3 edgeDirection = edgeVector / edgeLength;
    float t = glm::dot(ballToEdgeStart, edgeDirection);

    glm::vec3 closestPoint;
    if (t < 0) closestPoint = edge.start;
    else if (t > edgeLength) closestPoint = edge.end;
    else closestPoint = edge.start + edgeDirection * t;

    // Calculate collision normal, but ensure it's only in the XZ plane
    glm::vec3 collisionVector = position - closestPoint;
    glm::vec3 collisionNormal = glm::normalize(glm::vec3(collisionVector.x, 0.0f, collisionVector.z));

    // Only reflect the XZ components of velocity
    float velocityAlongNormal = glm::dot(glm::vec3(velocity.x, 0.0f, velocity.z), collisionNormal);
    if (velocityAlongNormal > 0) return; // Only bounce if moving toward edge

    // Calculate new velocity, preserving any existing y-component
    float yVelocity = velocity.y; // Store original y velocity
    glm::vec3 reflectionVector = velocity - (1.0f + restitution) * velocityAlongNormal * collisionNormal;
    velocity = glm::vec3(reflectionVector.x, yVelocity, reflectionVector.z);

    // Move ball out of edge to prevent sticking, but only in XZ plane
    float overlap = radius + edge.cushionWidth - glm::length(glm::vec2(collisionVector.x, collisionVector.z));
    if (overlap > 0) {
        position.x += collisionNormal.x * overlap;
        position.z += collisionNormal.z * overlap;
    }
}

void Ball::generateSphere() {
    const int segments = 32;
    const int rings = 16;

    // Generate vertices
    for (int ring = 0; ring <= rings; ring++) {
        float phi = M_PI * float(ring) / float(rings);
        for (int segment = 0; segment <= segments; segment++) {
            float theta = 2.0f * M_PI * float(segment) / float(segments);

            // Position
            float x = sin(phi) * cos(theta);
            float y = cos(phi);
            float z = sin(phi) * sin(theta);

            // Add vertex position (scaled by radius)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);

            // Add normal (same as position for unit sphere)
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }
    }

    // Generate indices
    for (int ring = 0; ring < rings; ring++) {
        for (int segment = 0; segment < segments; segment++) {
            unsigned int current = ring * (segments + 1) + segment;
            unsigned int next = current + segments + 1;

            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);

            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
    }
}

void Ball::setupBuffers() {
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

void Ball::cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}