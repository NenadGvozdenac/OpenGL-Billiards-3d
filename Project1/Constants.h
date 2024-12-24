#ifndef CONSTANTS_H
#define CONSTANTS_H

#define M_PI 3.14159265358979323846

enum GameStatus {
    NOT_STARTED = 0,
    PLAYING = 1,
    PAUSED = 2,
    FINISHED = 3
};

const float ballRadius = 0.08f;
const float tableHeight = 0.057f;
const float startCueAngle = 1.5708f;
const float frameTime = 1.0f / 120.0f;
#endif