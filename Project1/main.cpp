#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>

#include "TextRender.h"
#include "Shader.h"
#include "Camera.h"
#include "Ball.h"
#include "Cue.h"
#include "Button.h"
#include "Constants.h"

class BilliardsGame {
private:
    GameStatus gameStatus;
    int currentPlayer;
    int lowestBallNumber;
    bool firstBallHitCorrect;
    int firstBallHit;
    bool ballsPocketedThisTurn;
    bool foulThisTurn;
    glm::vec3 foulPosition;
    int playerWon;

    GLFWwindow* window;
    GLuint shaderProgram;
    GLuint overlayShaderProgram;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> overlayShader;

    GLuint tableVAO, tableVBO, tableEBO;
    GLuint edgesVAO, edgesVBO, edgesEBO;
    GLuint holesVAO, holesVBO, holesEBO;
    GLuint overlayVAO, overlayVBO;

    Camera camera;
    glm::mat4 projection;

    size_t edgeIndicesCount;
    size_t totalTableIndices;

    std::unique_ptr<Ball> cueBall;
    std::vector<std::unique_ptr<Ball>> balls;

    float cueAngle = startCueAngle;
    double lastMouseX = 0.0;
    bool firstMouse = true;
    GLuint cueVAO, cueVBO, cueEBO;

    std::unique_ptr<Cue> cue;

    std::vector<Edge> tableEdges;
    float lastFrameTime;

    bool canShoot = true;

    std::unique_ptr<TextRender> textRender;

    int selectedButton = 0;

    std::vector<Button> pauseButtons;
    std::vector<Button> endButtons;

    void handleBallCollision(Ball* ball1, Ball* ball2) {
        // If this is the first collision of the turn
        if (firstBallHit == -1) {
            // Determine which ball is the cue ball
            if (ball1 == cueBall.get()) {
                firstBallHit = ball2->number;
            }
            else if (ball2 == cueBall.get()) {
                firstBallHit = ball1->number;
            }

            // Check if the correct ball was hit
            firstBallHitCorrect = (firstBallHit == lowestBallNumber);
        }
    }

    int findLowestBallNumber() {
        int lowest = 9;

        for (const auto& ball : balls) {
            if (!ball->pocketed && ball->number < lowest) {
                lowest = ball->number;
            }
        }

        return lowest;
    }

    void checkBallsPocketed() {
        bool anyBallsPocketed = false;

        if (!cueBall->pocketed) {
            cueBall->checkHolePocketed();
            if (cueBall->pocketed) {
                // Handle scratch (cue ball pocketed)
                handleFoul();
            }
        }

        for (const auto& ball : balls) {
            bool wasPocketed = ball->pocketed;
            if (!wasPocketed) {
                ball->checkHolePocketed();
                if (ball->pocketed && !wasPocketed) {
                    anyBallsPocketed = true;
                    ballsPocketedThisTurn = true;
                }
            }
        }

        // Update lowest ball number if needed
        lowestBallNumber = findLowestBallNumber();
    }

    void handleFoul() {
        // Set the foul position to behind the head string
        foulPosition = glm::vec3(-1.0f, tableHeight, 0.0f);

        // Switch players
        currentPlayer = (currentPlayer == 1) ? 2 : 1;

        foulThisTurn = true;
    }

    void handleWin() {
        gameStatus = GameStatus::FINISHED;
        playerWon = currentPlayer;
    }

    void handleLose() {
        gameStatus = GameStatus::FINISHED;

        // Other player wins, but if they lost, its due to a foul which already changed them
        playerWon = currentPlayer;
    }

    void executeShot() {
        if (!canShoot || cueBall->pocketed) return;

        // Reset turn tracking variables
        firstBallHit = -1;
        firstBallHitCorrect = false;
        ballsPocketedThisTurn = false;
        foulThisTurn = false;

        // Calculate shot direction based on cue angle
        glm::vec3 direction(
            sin(cueAngle),
            0.0f,
            cos(cueAngle)
        );

        direction = glm::normalize(direction);
        cueBall->velocity = direction * cue->shotPower;

        cue->setShotPower(2.0f);
        cue->updateGeometry();
        canShoot = false;
    }

    static void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            BilliardsGame* game = static_cast<BilliardsGame*>(glfwGetWindowUserPointer(window));

            if (game->gameStatus == GameStatus::PAUSED) return;

            game->processMouse(xpos, ypos);
        }
    }

    void initOpenGL() {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // Get primary monitor and its video mode
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

        // Calculate window position for center of screen
        int windowWidth = 1200;
        int windowHeight = 1000;
        int xpos = (mode->width - windowWidth) / 2;
        int ypos = (mode->height - windowHeight) / 2;

        // Create window with calculated position and size
        window = glfwCreateWindow(windowWidth, windowHeight, "3D Billiards", NULL, NULL);
        glfwSetWindowPos(window, xpos, ypos);

        if (window == NULL) {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);

        if (glewInit() != GLEW_OK) {
            std::cout << "Failed to initialize GLEW" << std::endl;
            return;
        }

        glEnable(GL_DEPTH_TEST);
    }

    void initOverlay() {
        overlayShader = std::make_unique<Shader>("overlay.vert", "overlay.frag");
        overlayShaderProgram = overlayShader->shaderProgram;

        // Create full screen quad
        float vertices[] = {
            -1.0f,  1.0f,  // Top left
            -1.0f, -1.0f,  // Bottom left
             1.0f,  1.0f,  // Top right
             1.0f, -1.0f   // Bottom right
        };

        glGenVertexArrays(1, &overlayVAO);
        glGenBuffers(1, &overlayVBO);

        glBindVertexArray(overlayVAO);
        glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void initOverlays() {
        pauseButtons = {
            {"Continue", 500.0f, 450.0f, 200.0f, 40.0f},
            {"Exit", 500.0f, 350.0f, 200.0f, 40.0f}
        };

        endButtons = {
			{"Play Again", 500.0f, 450.0f, 200.0f, 40.0f},
			{"Exit", 500.0f, 350.0f, 200.0f, 40.0f}
		};
    }

    void initTextRender() {
        textRender = std::make_unique<TextRender>("font/Roboto-Regular.ttf", "text.vert", "text.frag", 20);
    }

    void initializeBalls() {
        // Define colors for numbered balls
        std::vector<glm::vec3> ballColors = {
            glm::vec3(1.0f, 1.0f, 0.0f),    // 1-ball: Yellow
            glm::vec3(0.0f, 0.0f, 1.0f),    // 2-ball: Blue
            glm::vec3(1.0f, 0.0f, 0.0f),    // 3-ball: Red
            glm::vec3(0.5f, 0.0f, 0.5f),    // 4-ball: Purple
            glm::vec3(1.0f, 0.5f, 0.0f),    // 5-ball: Orange
            glm::vec3(0.0f, 0.7f, 0.0f),    // 6-ball: Green
            glm::vec3(0.5f, 0.0f, 0.0f),    // 7-ball: Maroon
            glm::vec3(0.0f, 0.0f, 0.0f),    // 8-ball: Black
            glm::vec3(1.0f, 1.0f, 0.4f)     // 9-ball: Yellow with white stripe
        };

        // Calculate positions for diamond rack formation
        float row_spacing = ballRadius * 2.1f;  // Slightly more than diameter for tight rack
        float col_spacing = row_spacing * 0.866f;  // cos(30°) for equilateral triangle spacing

        // Rack position (centered on the table, appropriate distance from cue ball)
        float rackCenterX = 1.0f;  // Positive X for opposite side from cue ball
        float rackCenterZ = 0.0f;  // Centered on Z axis

        // Define rack positions (from front to back)
        std::vector<glm::vec2> rackPositions = {
            // Front (1-ball)
            glm::vec2(rackCenterX - col_spacing * 2, rackCenterZ),
            // Second row (2,3)
            glm::vec2(rackCenterX - col_spacing, rackCenterZ - row_spacing / 2),
            glm::vec2(rackCenterX - col_spacing, rackCenterZ + row_spacing / 2),
            // Third row (4,9,5)
            glm::vec2(rackCenterX, rackCenterZ - row_spacing),
            glm::vec2(rackCenterX, rackCenterZ),  // 9-ball in center
            glm::vec2(rackCenterX, rackCenterZ + row_spacing),
            // Fourth row (6,7)
            glm::vec2(rackCenterX + col_spacing, rackCenterZ - row_spacing / 2),
            glm::vec2(rackCenterX + col_spacing, rackCenterZ + row_spacing / 2),
            // Back (8-ball)
            glm::vec2(rackCenterX + col_spacing * 2, rackCenterZ)
        };

        balls.clear();

        // Create the 9 numbered balls
        for (int i = 0; i < 9; i++) {
            balls.push_back(std::make_unique<Ball>(
                rackPositions[i].x,
                tableHeight,
                rackPositions[i].y,
                ballRadius,
                ballColors[i],
                i + 1
            ));
        }
    }

    void initializeEdges() {
        const float cushionWidth = 0.125f;
        const float holeRadius = 0.15f;
        const float holeOffset = -holeRadius * 1.4142f + 0.1f; // sqrt(2) * holeRadius for diagonal offset

        // Define the table edges with normals pointing inward
        // Top edges
        tableEdges.push_back(Edge(
            glm::vec3(-2.0f + holeOffset, 0.0f, 1.0f - holeOffset),  // Start after top-left hole
            glm::vec3(-holeOffset, 0.0f, 1.0f - holeOffset),         // End before top-middle hole
            glm::vec3(0.0f, 0.0f, -1.0f),
            cushionWidth
        ));

        tableEdges.push_back(Edge(
            glm::vec3(holeOffset, 0.0f, 1.0f - holeOffset),          // Start after top-middle hole
            glm::vec3(2.0f - holeOffset, 0.0f, 1.0f - holeOffset),   // End before top-right hole
            glm::vec3(0.0f, 0.0f, -1.0f),
            cushionWidth
        ));

        // Bottom edges
        tableEdges.push_back(Edge(
            glm::vec3(-2.0f + holeOffset, 0.0f, -1.0f + holeOffset), // Start after bottom-left hole
            glm::vec3(-holeOffset, 0.0f, -1.0f + holeOffset),        // End before bottom-middle hole
            glm::vec3(0.0f, 0.0f, 1.0f),
            cushionWidth
        ));

        tableEdges.push_back(Edge(
            glm::vec3(holeOffset, 0.0f, -1.0f + holeOffset),         // Start after bottom-middle hole
            glm::vec3(2.0f - holeOffset, 0.0f, -1.0f + holeOffset),  // End before bottom-right hole
            glm::vec3(0.0f, 0.0f, 1.0f),
            cushionWidth
        ));

        // Left edge
        tableEdges.push_back(Edge(
            glm::vec3(-2.0f + holeOffset, 0.0f, 1.0f - holeOffset),  // Start after top-left hole
            glm::vec3(-2.0f + holeOffset, 0.0f, -1.0f + holeOffset), // End before bottom-left hole
            glm::vec3(1.0f, 0.0f, 0.0f),
            cushionWidth
        ));

        // Right edge
        tableEdges.push_back(Edge(
            glm::vec3(2.0f - holeOffset, 0.0f, 1.0f - holeOffset),   // Start after top-right hole
            glm::vec3(2.0f - holeOffset, 0.0f, -1.0f + holeOffset),  // End before bottom-right hole
            glm::vec3(-1.0f, 0.0f, 0.0f),
            cushionWidth
        ));
    }

    void setupHoles() {
        const float holeRadius = 0.15f;
        const int segments = 32;
        const float depth = -0.05f;
        const float holeElevation = 0.06f;

        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        // Hole positions
        std::vector<glm::vec2> holePositions = {
            glm::vec2(-2.0f, 1.0f),    // Top-left corner
            glm::vec2(0.0f, 1.0f),     // Top-middle
            glm::vec2(2.0f, 1.0f),     // Top-right corner
            glm::vec2(-2.0f, -1.0f),   // Bottom-left corner
            glm::vec2(0.0f, -1.0f),    // Bottom-middle
            glm::vec2(2.0f, -1.0f)     // Bottom-right corner
        };

        // Generate vertices and indices for each hole
        for (size_t h = 0; h < holePositions.size(); h++) {
            // Store the starting vertex index for this hole
            int baseVertex = vertices.size() / 6;

            // Add center vertex at the top
            vertices.push_back(holePositions[h].x);  // x
            vertices.push_back(holeElevation);       // y
            vertices.push_back(holePositions[h].y);  // z
            vertices.push_back(0.0f);                // normal x
            vertices.push_back(-1.0f);               // normal y
            vertices.push_back(0.0f);                // normal z

            // Add center vertex at the bottom
            vertices.push_back(holePositions[h].x);  // x
            vertices.push_back(depth);               // y
            vertices.push_back(holePositions[h].y);  // z
            vertices.push_back(0.0f);                // normal x
            vertices.push_back(-1.0f);               // normal y
            vertices.push_back(0.0f);                // normal z

            // Generate circle vertices at top and bottom
            for (int i = 0; i <= segments; i++) {
                float angle = 2.0f * M_PI * float(i) / float(segments);
                float x = holePositions[h].x + holeRadius * cos(angle);
                float z = holePositions[h].y + holeRadius * sin(angle);

                // Top rim vertex
                vertices.push_back(x);
                vertices.push_back(holeElevation);
                vertices.push_back(z);
                vertices.push_back(cos(angle));
                vertices.push_back(-1.0f);
                vertices.push_back(sin(angle));

                // Bottom rim vertex
                vertices.push_back(x);
                vertices.push_back(depth);
                vertices.push_back(z);
                vertices.push_back(cos(angle));
                vertices.push_back(-1.0f);
                vertices.push_back(sin(angle));

                if (i < segments) {
                    // Top face triangles
                    indices.push_back(baseVertex);  // Center top
                    indices.push_back(baseVertex + 2 + i * 2);
                    indices.push_back(baseVertex + 2 + (i + 1) * 2);

                    // Bottom face triangles
                    indices.push_back(baseVertex + 1);  // Center bottom
                    indices.push_back(baseVertex + 3 + i * 2);
                    indices.push_back(baseVertex + 3 + (i + 1) * 2);

                    // Side triangles (wall)
                    indices.push_back(baseVertex + 2 + i * 2);
                    indices.push_back(baseVertex + 3 + i * 2);
                    indices.push_back(baseVertex + 2 + (i + 1) * 2);

                    indices.push_back(baseVertex + 3 + i * 2);
                    indices.push_back(baseVertex + 3 + (i + 1) * 2);
                    indices.push_back(baseVertex + 2 + (i + 1) * 2);
                }
            }
        }

        glGenVertexArrays(1, &holesVAO);
        glGenBuffers(1, &holesVBO);
        glGenBuffers(1, &holesEBO);

        glBindVertexArray(holesVAO);

        glBindBuffer(GL_ARRAY_BUFFER, holesVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, holesEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void setupTable() {
        const float legWidth = 0.1f;
        const float legHeight = 0.7f;
        // Vertices for the table (position, normal)
        std::vector<float> vertices = {
            // Original table vertices (top surface, sides, bottom)
            // Top surface
            -2.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,  // 0
             2.0f,  0.0f,  1.0f,   0.0f,  1.0f,  0.0f,  // 1
             2.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,  // 2
            -2.0f,  0.0f, -1.0f,   0.0f,  1.0f,  0.0f,  // 3

            // Front face
            -2.0f,  0.0f,  1.0f,   0.0f,  0.0f,  1.0f,  // 4
             2.0f,  0.0f,  1.0f,   0.0f,  0.0f,  1.0f,  // 5
             2.0f, -0.2f,  1.0f,   0.0f,  0.0f,  1.0f,  // 6
            -2.0f, -0.2f,  1.0f,   0.0f,  0.0f,  1.0f,  // 7

            // Back face
            -2.0f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,  // 8
             2.0f,  0.0f, -1.0f,   0.0f,  0.0f, -1.0f,  // 9
             2.0f, -0.2f, -1.0f,   0.0f,  0.0f, -1.0f,  // 10
            -2.0f, -0.2f, -1.0f,   0.0f,  0.0f, -1.0f,  // 11

            // Left face
            -2.0f,  0.0f,  1.0f,  -1.0f,  0.0f,  0.0f,  // 12
            -2.0f,  0.0f, -1.0f,  -1.0f,  0.0f,  0.0f,  // 13
            -2.0f, -0.2f, -1.0f,  -1.0f,  0.0f,  0.0f,  // 14
            -2.0f, -0.2f,  1.0f,  -1.0f,  0.0f,  0.0f,  // 15

            // Right face
             2.0f,  0.0f,  1.0f,   1.0f,  0.0f,  0.0f,  // 16
             2.0f,  0.0f, -1.0f,   1.0f,  0.0f,  0.0f,  // 17
             2.0f, -0.2f, -1.0f,   1.0f,  0.0f,  0.0f,  // 18
             2.0f, -0.2f,  1.0f,   1.0f,  0.0f,  0.0f,  // 19

             // Bottom face
             -2.0f, -0.2f,  1.0f,   0.0f, -1.0f,  0.0f,  // 20
              2.0f, -0.2f,  1.0f,   0.0f, -1.0f,  0.0f,  // 21
              2.0f, -0.2f, -1.0f,   0.0f, -1.0f,  0.0f,  // 22
             -2.0f, -0.2f, -1.0f,   0.0f, -1.0f,  0.0f,  // 23

             // Table Legs (4 corners + 2 middle supports)
             // Front Left Leg
             -1.9f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 24
             -1.8f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 25
             -1.8f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 26
             -1.9f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 27
             -1.9f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 28
             -1.8f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 29
             -1.8f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 30
             -1.9f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 31

             // Front Right Leg
              1.9f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 32
              1.8f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 33
              1.8f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 34
              1.9f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 35
              1.9f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 36
              1.8f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 37
              1.8f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 38
              1.9f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 39

              // Back Left Leg
              -1.9f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 40
              -1.8f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 41
              -1.8f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 42
              -1.9f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 43
              -1.9f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 44
              -1.8f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 45
              -1.8f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f,  // 46
              -1.9f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f,  // 47

              // Back Right Leg
               1.9f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 48
               1.8f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 49
               1.8f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 50
               1.9f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 51
               1.9f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 52
               1.8f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 53
               1.8f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f,  // 54
               1.9f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f,  // 55

               // Middle Support Legs
               // Front Middle
                0.1f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 56
               -0.1f, -0.2f,  0.9f,   0.0f,  1.0f,  0.0f,  // 57
               -0.1f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 58
                0.1f, -0.2f,  0.8f,   0.0f,  1.0f,  0.0f,  // 59
                0.1f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 60
               -0.1f, -legHeight,  0.9f,   0.0f,  1.0f,  0.0f,  // 61
               -0.1f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 62
                0.1f, -legHeight,  0.8f,   0.0f,  1.0f,  0.0f,  // 63

                // Back Middle
                 0.1f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 64
                -0.1f, -0.2f, -0.9f,   0.0f,  1.0f,  0.0f,  // 65
                -0.1f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 66
                 0.1f, -0.2f, -0.8f,   0.0f,  1.0f,  0.0f,  // 67
                 0.1f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 68
                -0.1f, -legHeight, -0.9f,   0.0f,  1.0f,  0.0f,  // 69
                -0.1f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f,  // 70
                 0.1f, -legHeight, -0.8f,   0.0f,  1.0f,  0.0f   // 71
        };

        // Indices for the table
        std::vector<unsigned int> indices = {
            // Original table indices
            0, 1, 2,    0, 2, 3,      // Top
            4, 5, 6,    4, 6, 7,      // Front
            8, 9, 10,   8, 10, 11,    // Back
            12, 13, 14, 12, 14, 15,   // Left
            16, 17, 18, 16, 18, 19,   // Right
            20, 21, 22, 20, 22, 23    // Bottom
        };

        auto addLegIndices = [&indices](unsigned int baseIndex) {
            // Front face
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 5);
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 5);
            indices.push_back(baseIndex + 4);

            // Back face
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 7);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 7);
            indices.push_back(baseIndex + 6);

            // Left face
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 3);
            indices.push_back(baseIndex + 7);
            indices.push_back(baseIndex + 0);
            indices.push_back(baseIndex + 7);
            indices.push_back(baseIndex + 4);

            // Right face
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 2);
            indices.push_back(baseIndex + 6);
            indices.push_back(baseIndex + 1);
            indices.push_back(baseIndex + 6);
            indices.push_back(baseIndex + 5);

            // Bottom face
            indices.push_back(baseIndex + 4);
            indices.push_back(baseIndex + 5);
            indices.push_back(baseIndex + 6);
            indices.push_back(baseIndex + 4);
            indices.push_back(baseIndex + 6);
            indices.push_back(baseIndex + 7);
            };

        // Add indices for all legs
        addLegIndices(24);  // Front Left Leg
        addLegIndices(32);  // Front Right Leg
        addLegIndices(40);  // Back Left Leg
        addLegIndices(48);  // Back Right Leg
        addLegIndices(56);  // Front Middle Support
        addLegIndices(64);  // Back Middle Support

        glGenVertexArrays(1, &tableVAO);
        glGenBuffers(1, &tableVBO);
        glGenBuffers(1, &tableEBO);

        glBindVertexArray(tableVAO);

        glBindBuffer(GL_ARRAY_BUFFER, tableVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, tableEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        totalTableIndices = indices.size();
    }

    void setupEdges() {
        const float cushionWidth = 0.125f;
        const float cushionHeight = 0.02f;
        const float holeRadius = 0.15f;

        // Hole positions (reused from hole setup)
        std::vector<glm::vec2> holePositions = {
            glm::vec2(-2.0f, 1.0f),    // Top-left (0)
            glm::vec2(0.0f, 1.0f),     // Top-middle (1)
            glm::vec2(2.0f, 1.0f),     // Top-right (2)
            glm::vec2(-2.0f, -1.0f),   // Bottom-left (3)
            glm::vec2(0.0f, -1.0f),    // Bottom-middle (4)
            glm::vec2(2.0f, -1.0f)     // Bottom-right (5)
        };

        // Define edge segments (start and end positions)
        std::vector<std::pair<glm::vec2, glm::vec2>> edgeSegments = {
            // Top cushions
            {glm::vec2(-2.0f + holeRadius, 1.0f), glm::vec2(-holeRadius, 1.0f)},      // Top-left to top-middle
            {glm::vec2(holeRadius, 1.0f), glm::vec2(2.0f - holeRadius, 1.0f)},        // Top-middle to top-right

            // Bottom cushions
            {glm::vec2(-2.0f + holeRadius, -1.0f), glm::vec2(-holeRadius, -1.0f)},    // Bottom-left to bottom-middle
            {glm::vec2(holeRadius, -1.0f), glm::vec2(2.0f - holeRadius, -1.0f)},      // Bottom-middle to bottom-right

            // Side cushions
            {glm::vec2(-2.0f, 1.0f - holeRadius), glm::vec2(-2.0f, -1.0f + holeRadius)},  // Left side
            {glm::vec2(2.0f, 1.0f - holeRadius), glm::vec2(2.0f, -1.0f + holeRadius)}     // Right side
        };

        std::vector<float> vertices;
        std::vector<int> indices;
        int vertexCount = 0;

        // Generate vertices and indices for each edge segment
        for (int i = 0; i < edgeSegments.size(); i++) {
            const auto& segment = edgeSegments[i];
            glm::vec2 start = segment.first;
            glm::vec2 end = segment.second;
            glm::vec2 direction = glm::normalize(end - start);
            glm::vec2 normal(-direction.y, direction.x);

            // Calculate inner points
            glm::vec2 startInner, endInner;

            if (i == 4 || i == 2 || i == 3) { // Left side cushion OR top cushion - invert normal direction
                startInner = start - normal * cushionWidth;
                endInner = end - normal * cushionWidth;
            }
            else {
                startInner = start + normal * cushionWidth;
                endInner = end + normal * cushionWidth;
            }

            // Outer face
            vertices.insert(vertices.end(), {
                // Outer front vertices
                start.x, 0.0f, start.y,                0.0f, 0.0f, 1.0f,
                end.x, 0.0f, end.y,                    0.0f, 0.0f, 1.0f,
                end.x, cushionHeight, end.y,           0.0f, 0.0f, 1.0f,
                start.x, cushionHeight, start.y,       0.0f, 0.0f, 1.0f,

                // Inner back vertices
                startInner.x, 0.0f, startInner.y,      0.0f, 0.0f, -1.0f,
                endInner.x, 0.0f, endInner.y,          0.0f, 0.0f, -1.0f,
                endInner.x, cushionHeight, endInner.y, 0.0f, 0.0f, -1.0f,
                startInner.x, cushionHeight, startInner.y, 0.0f, 0.0f, -1.0f
                });

            int baseIndex = vertexCount;
            indices.insert(indices.end(), {
                // Front face
                baseIndex + 0, baseIndex + 1, baseIndex + 2,
                baseIndex + 0, baseIndex + 2, baseIndex + 3,
                // Back face
                baseIndex + 4, baseIndex + 5, baseIndex + 6,
                baseIndex + 4, baseIndex + 6, baseIndex + 7,
                // Top face
                baseIndex + 3, baseIndex + 2, baseIndex + 6,
                baseIndex + 3, baseIndex + 6, baseIndex + 7,
                // End faces
                baseIndex + 0, baseIndex + 3, baseIndex + 7,
                baseIndex + 0, baseIndex + 7, baseIndex + 4,
                baseIndex + 1, baseIndex + 2, baseIndex + 6,
                baseIndex + 1, baseIndex + 6, baseIndex + 5
                });

            vertexCount += 8;
        }

        edgeIndicesCount = indices.size();

        // Create and bind buffers
        glGenVertexArrays(1, &edgesVAO);
        glGenBuffers(1, &edgesVBO);
        glGenBuffers(1, &edgesEBO);

        glBindVertexArray(edgesVAO);

        glBindBuffer(GL_ARRAY_BUFFER, edgesVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, edgesEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Set vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void processMouse(double xpos, double ypos) {
        if (firstMouse) {
            lastMouseX = xpos;
            firstMouse = false;
            return;
        }

        float xoffset = xpos - lastMouseX;
        lastMouseX = xpos;

        // Convert mouse movement to rotation
        float sensitivity = 0.005f;
        cueAngle += xoffset * sensitivity;
    }

    void renderCue() {
        glm::mat4 model = glm::mat4(1.0f);

        // Position the cue at the cue ball
        model = glm::translate(model, cueBall->position);

        // Rotate around the cue ball
        model = glm::rotate(model, cueAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        // Move the cue back so it doesn't intersect with the ball
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -0.15f));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // Set cue color (wooden brown)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.545f, 0.271f, 0.075f);

        glBindVertexArray(cue->VAO);
        glDrawElements(GL_TRIANGLES, cue->indices.size(), GL_UNSIGNED_INT, 0);
    }

    void renderTable() {
        glUseProgram(shaderProgram);

        // Enhanced lighting setup
        glm::vec3 lightPos(2.0f, 5.0f, 2.0f);
        glm::vec3 lightColor(1.0f, 1.0f, 1.0f);

        // Set lighting uniforms
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(shaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(camera.position));

        // Set transformation matrices
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(camera.getViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(tableVAO);

        // Draw table top (green felt)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.5f, 0.0f);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);  // Original table surface indices

        // Draw table legs (dark brown)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.1f, 0.05f);
        glDrawElements(GL_TRIANGLES, totalTableIndices - 36, GL_UNSIGNED_INT, (void*)(36 * sizeof(unsigned int)));

        // Draw edges (brown)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.545f, 0.271f, 0.075f);
        glBindVertexArray(edgesVAO);
        glDrawElements(GL_TRIANGLES, edgeIndicesCount, GL_UNSIGNED_INT, 0);

        // Draw holes (black)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.0f, 0.0f);
        glBindVertexArray(holesVAO);
        glDrawElements(GL_TRIANGLES, 6 * 32 * 6 * 2, GL_UNSIGNED_INT, 0);
    }

    void renderBalls() {
        for (const auto& ball : balls) {
            if(ball->pocketed) continue;

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, ball->position);
            model = glm::scale(model, glm::vec3(ball->radius));

            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(ball->color));

            glBindVertexArray(ball->VAO);
            glDrawElements(GL_TRIANGLES, ball->indices.size(), GL_UNSIGNED_INT, 0);
        }
    }

    void renderCueBall() {
        if(cueBall->pocketed) return;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, cueBall->position);
        model = glm::scale(model, glm::vec3(cueBall->radius));

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

        // Set white color for cue ball
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);

        glBindVertexArray(cueBall->VAO);
        glDrawElements(GL_TRIANGLES, cueBall->indices.size(), GL_UNSIGNED_INT, 0);
    }

    void renderPauseOverlay() {
        if (gameStatus != GameStatus::PAUSED) return;

        // Render "PAUSE" text
        textRender->RenderText("PAUSE",
            500.0f,
            550.0f,
            2.0f,
            glm::vec3(1.0f, 1.0f, 1.0f));

        // Render buttons
        for (size_t i = 0; i < pauseButtons.size(); i++) {
            const auto& button = pauseButtons[i];

            // Determine button color based on selection
            glm::vec3 color = (i == selectedButton)
                ? glm::vec3(1.0f, 1.0f, 0.0f)  // Yellow for selected
                : glm::vec3(1.0f, 1.0f, 1.0f); // White for unselected

            textRender->RenderText(button.text,
                button.x,
                button.y,
                1.5f,
                color);
        }

        // Render overlay background
        renderOverlayBackground();
    }

    void renderEndScreen() {
		if (gameStatus != GameStatus::FINISHED) return;

		// Render "GAME OVER" text
		textRender->RenderText("GAME OVER",
            			500.0f,
            			550.0f,
            			2.0f,
            			glm::vec3(1.0f, 1.0f, 1.0f));

		// Render winner text
		textRender->RenderText("Player " + std::to_string(playerWon) + " wins!",
            			500.0f,
            			500.0f,
            			1.5f,
            			glm::vec3(1.0f, 1.0f, 1.0f));

		// Render buttons
        for (size_t i = 0; i < endButtons.size(); i++) {
			const auto& button = endButtons[i];

			// Determine button color based on selection
			glm::vec3 color = (i == selectedButton)
				? glm::vec3(1.0f, 1.0f, 0.0f)  // Yellow for selected
				: glm::vec3(1.0f, 1.0f, 1.0f); // White for unselected

			textRender->RenderText(button.text,
                				button.x,
                				button.y,
                				1.5f,
                				color);
		}

		// Render overlay background
		renderOverlayBackground();
	}

    void renderOverlayBackground() {
        // Enable blending for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Use overlay shader
        glUseProgram(overlayShaderProgram);

        // Draw full screen quad
        glBindVertexArray(overlayVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    void renderText() {
        textRender->RenderText("Nenad Gvozdenac", 980.0f, 825.0f, 1.25f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("RA 133/2021", 980.0f, 800.0f, 1.25f, glm::vec3(1.0f, 1.0f, 1.0f));

        textRender->RenderText("Current player: Player " + std::to_string(currentPlayer), 5.0f, 825.0f, 1.25f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("Next ball: " + std::to_string(lowestBallNumber), 5.0f, 790.0f, 1.25f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("Controls: ", 5.0f, 755.0f, 1.25f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("CTRL + 1, 2, 3, 4, 5 - Switch camera", 5.0f, 730.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("ALT + 1, 2, 3 - Zoom camera", 5.0f, 705.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("Mouse - Rotate cue", 5.0f, 680.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("1, 2, 3, 4, 5 - Switch hit strength", 5.0f, 655.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        textRender->RenderText("Space - Hit cue ball", 5.0f, 630.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        textRender->RenderText("Current camera: " + getCurrentCamera(), 5.0f, 30.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));

        if (foulThisTurn) {
            textRender->RenderText("Foul committed! Opponent's turn!", 450.0f, 630.0f, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }

    void render() {
        glClearColor(0.1f, 0.3f, 0.3f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderTable();
        renderCueBall();
        renderBalls();

        if (canShoot && !cueBall->pocketed) {
            renderCue();
        }

        if (gameStatus == GameStatus::PLAYING || gameStatus == GameStatus::NOT_STARTED) {
            renderText();
        }

        if (gameStatus == GameStatus::PAUSED) {
            renderOverlayBackground();
            renderPauseOverlay();
        }

        if (gameStatus == GameStatus::FINISHED) {
            renderOverlayBackground();
			renderEndScreen();
        }
    }

    std::string getCurrentCamera() {
        switch (camera.currentView) {
            case 1:
                return "Angle #1";
            break;
            case 2:
				return "Angle #2";
            break;
			case 3:
                return "Angle #3";
			break;
            case 4:
				return "Angle #4";
            break;
            case 5:
				return "Top-View";
            break;
        }
    }

    void resetCue() {
		cueAngle = 1.5708f;
        cue->setShotPower(2.0f);
        cue->updateGeometry();
	}

    void resetCueBall() {
        // Reset cue ball position to starting position
        cueBall->position = glm::vec3(-1.0f, tableHeight, 0.0f);
        cueBall->velocity = glm::vec3(0.0f);
        cueBall->pocketed = false;
        resetCue();
    }

    void updatePhysics(float frameTime) {
        float deltaTime = frameTime;
        bool allBallsStopped = true;

        if (!cueBall->pocketed) {
            cueBall->update(deltaTime);
            if (glm::length(cueBall->velocity) > 0.01f) {
                allBallsStopped = false;
            }
        }

        for (const auto& ball : balls) {
            if (!ball->pocketed) {
                ball->update(deltaTime);
                if (glm::length(ball->velocity) > 0.01f) {
                    allBallsStopped = false;
                }
            }
        }

        // Only check for pocketed balls while balls are in motion
        if (!allBallsStopped) {
            checkBallsPocketed();
        }

        // When all balls stop, evaluate the turn
        if (allBallsStopped && !canShoot) {
            evaluateTurn();
        }

        // Ball collision physics
        if (!cueBall->pocketed) {
            for (const auto& ball : balls) {
                if (!ball->pocketed && cueBall->checkCollision(ball.get())) {
                    handleBallCollision(cueBall.get(), ball.get());
                    cueBall->resolveCollision(ball.get());
                }
            }
        }

        // Rest of the existing physics code...
        for (size_t i = 0; i < balls.size(); i++) {
            if (balls[i]->pocketed) continue;
            for (size_t j = i + 1; j < balls.size(); j++) {
                if (balls[j]->pocketed) continue;
                if (balls[i]->checkCollision(balls[j].get())) {
                    balls[i]->resolveCollision(balls[j].get());
                }
            }
        }

        // Allow shooting again if all balls have stopped
        if (allBallsStopped) {
            canShoot = true;
        }

        if (!cueBall->pocketed) {
            for (const Edge& edge : tableEdges) {
                if (cueBall->checkEdgeCollision(edge)) {
                    cueBall->resolveEdgeCollision(edge);
                }
            }
        }

        for (const auto& ball : balls) {
            if (!ball->pocketed) {
                for (const Edge& edge : tableEdges) {
                    if (ball->checkEdgeCollision(edge)) {
                        ball->resolveEdgeCollision(edge);
                    }
                }
            }
        }
    }

    void evaluateTurn() {
        // If wrong ball was hit first, it's a foul
        if (!firstBallHitCorrect) {
            handleFoul();
        }
        
        // If no balls were pocketed, switch players
        if (!ballsPocketedThisTurn && firstBallHitCorrect) {
            currentPlayer = (currentPlayer == 1) ? 2 : 1;
        }

        // If 9-ball was pocketed, its a win
        if (balls.at(8)->pocketed && foulThisTurn) {
            handleLose();
        }
        else if (balls.at(8)->pocketed) {
			handleWin();
		}

        // If there was a foul this turn, reset the cue ball position
        if (foulThisTurn || cueBall->pocketed) {
            resetCueBall();
        }

        // Reset turn tracking variables
        firstBallHit = -1;
        firstBallHitCorrect = false;
        ballsPocketedThisTurn = false;
    }

    void openPauseOverlay() {
        gameStatus = GameStatus::PAUSED;
        selectedButton = 0;
    }

    void closePauseOverlay() {
        gameStatus = GameStatus::PLAYING;
	}
    
    void restartGame() {
        // Reset game state
        currentPlayer = 1;
        lowestBallNumber = 1;
        firstBallHit = -1;
        firstBallHitCorrect = false;
        ballsPocketedThisTurn = false;
        foulThisTurn = false;
        playerWon = 0;

        initializeBalls();
        resetCueBall();
        resetCue();

        gameStatus = GameStatus::NOT_STARTED;
    }

    void handleInput() {
        if(gameStatus != GameStatus::PLAYING && gameStatus != GameStatus::NOT_STARTED) return;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            openPauseOverlay();

        // Check for CTRL + number combinations
        bool ctrlPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

        if (ctrlPressed) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) camera.setView(1);
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) camera.setView(2);
            if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) camera.setView(3);
            if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) camera.setView(4);
            if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) camera.setView(5);

            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) resetCue();
        }

        bool altPressed = glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
			glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;

        if (altPressed) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) camera.setZoom(0);
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) camera.setZoom(1);
            if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) camera.setZoom(2);
        }

        // Check for shot power input
        if (canShoot) {
            if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS && !altPressed && !ctrlPressed) {
                cue->setShotPower(2.0f);
                cue->updateGeometry();
            }
            if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS && !altPressed && !ctrlPressed) {
				cue->setShotPower(4.0f);
				cue->updateGeometry();
			}
            if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS && !altPressed && !ctrlPressed) {
                cue->setShotPower(6.0f);
                cue->updateGeometry();
            }
            if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS && !altPressed && !ctrlPressed) {
				cue->setShotPower(8.0f);
				cue->updateGeometry();
			}
            if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS && !altPressed && !ctrlPressed) {
				cue->setShotPower(10.0f);
				cue->updateGeometry();
			}

            // Execute shot on spacebar press
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                executeShot();
            }
        }
    }

    void handlePauseInput(GLFWwindow* window) {
        if (gameStatus != GameStatus::PAUSED) return;

        // Get mouse position
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // Convert screen coordinates if necessary
        // You might need to adjust this based on your coordinate system
        int windowHeight;
        glfwGetWindowSize(window, nullptr, &windowHeight);
        mouseY = windowHeight - mouseY - 75; // Flip Y coordinate if needed

        // Check button hover
        for (size_t i = 0; i < pauseButtons.size(); i++) {
            if (pauseButtons[i].isHovered(mouseX, mouseY)) {
                selectedButton = i;

                // Handle click
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
                    if (i == 0) { // Continue
                        closePauseOverlay();
                    }
                    else if (i == 1) { // Exit
                        glfwSetWindowShouldClose(window, true);
                    }
                }
            }
        }

        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (selectedButton == 0) {
                closePauseOverlay();
            }
            else if (selectedButton == 1) {
                glfwSetWindowShouldClose(window, true);
            }
        }
    }

    void handleEndInput(GLFWwindow* window) {
		if (gameStatus != GameStatus::FINISHED) return;

		// Get mouse position
		double mouseX, mouseY;
		glfwGetCursorPos(window, &mouseX, &mouseY);

		// Convert screen coordinates if necessary
		int windowHeight;
		glfwGetWindowSize(window, nullptr, &windowHeight);
		mouseY = windowHeight - mouseY - 75; // Flip Y coordinate if needed

		// Check button hover
        for (size_t i = 0; i < endButtons.size(); i++) {
            if (endButtons[i].isHovered(mouseX, mouseY)) {
				selectedButton = i;

				// Handle click
                if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
					if (i == 0) { // Restart
                        restartGame();
                    }
					else if (i == 1) { // Exit
                        glfwSetWindowShouldClose(window, true);
                    }
				}
			}
		}

        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (selectedButton == 0) {
				restartGame();
			}
            else if (selectedButton == 1) {
				glfwSetWindowShouldClose(window, true);
			}
		}
	}

    void createShaders() {
        shader = std::make_unique<Shader>("basic.vert", "basic.frag");
        shaderProgram = shader->shaderProgram;
    }

public:
    BilliardsGame() {
        initOpenGL();
        initTextRender();
        initOverlay();
        initOverlays();

        // Initialize player-related variables
        currentPlayer = 1;  // Player 1 starts
        lowestBallNumber = 1;  // Start with ball 1
        firstBallHit = -1;
        firstBallHitCorrect = false;
        ballsPocketedThisTurn = false;
        foulPosition = glm::vec3(-1.0f, tableHeight, 0.0f);

        gameStatus = GameStatus::NOT_STARTED;

        initializeEdges();
        lastFrameTime = glfwGetTime();

        // Set the user pointer for the window to this instance
        glfwSetWindowUserPointer(window, this);

        // Set mouse callback function
        glfwSetCursorPosCallback(window, mouseCallback);

        createShaders();
        setupTable();
        setupEdges();
        setupHoles();

        // Initialize projection matrix with new window dimensions
        projection = glm::perspective(glm::radians(45.0f), 1200.0f / 1000.0f, 0.1f, 100.0f);

        // Initialize cue ball

        // Create cue ball (white)
        cueBall = std::make_unique<Ball>(-1.2f, tableHeight, 0.0f, ballRadius, glm::vec3(1.0f, 1.0f, 1.0f), 0);
        cue = std::make_unique<Cue>(2.5f, 0.025f);

        // Initialize balls
        initializeBalls();
    }

    void run() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        double lastTime = glfwGetTime();
        double accumulator = 0.0;

        while (!glfwWindowShouldClose(window)) {
            double currentTime = glfwGetTime();
            accumulator += currentTime - lastTime;
            lastTime = currentTime;

            handleInput();
            handlePauseInput(window);
            handleEndInput(window);

            while (accumulator >= frameTime) {
                updatePhysics(frameTime);
                accumulator -= frameTime;
            }

            render();
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
        cleanup();
    }

    void cleanup() {
        for (const auto& ball : balls) {
            ball->cleanup();
        }

        cueBall->cleanup();

        cue->cleanup();

        balls.clear();

        glDeleteVertexArrays(1, &tableVAO);
        glDeleteBuffers(1, &tableVBO);
        glDeleteBuffers(1, &tableEBO);
        glDeleteVertexArrays(1, &edgesVAO);
        glDeleteBuffers(1, &edgesVBO);
        glDeleteBuffers(1, &edgesEBO);
        glDeleteProgram(shaderProgram);
        glfwTerminate();
    }
};

int main() {
    BilliardsGame game;
    game.run();
    return 0;
}