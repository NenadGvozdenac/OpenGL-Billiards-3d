#include "Camera.h"

Camera::Camera() : position(0.0f, 4.0f, 4.0f),
	target(0.0f, 0.0f, 0.0f),
	up(0.0f, 1.0f, 0.0f),
	currentZoom(0),
	currentView(1) {}

void Camera::setView(int viewNumber) {
	currentView = viewNumber;
	switch (viewNumber) {
	case 1: // (default)
		position = glm::vec3(0.0f, 4.f + 1.0f * currentZoom, 4.0f + 1.0f * currentZoom);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 2:
		position = glm::vec3(5.0f + 1.0f * currentZoom, 3.0f + 1.0f * currentZoom, 0.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 3:
		position = glm::vec3(0.0f, 4.0f + 1.0f * currentZoom, -4.0f - 1.0f * currentZoom);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 4:
		position = glm::vec3(-5.0f - 1.0f * currentZoom, 3.0f + 1.0f * currentZoom, 0.0f);
		up = glm::vec3(0.0f, 1.0f, 0.0f);
		break;
	case 5:
		position = glm::vec3(0.0f, 6.0f + 1.0f * currentZoom, 0.0f);
		up = glm::vec3(0.0f, 0.0f, -1.0f);
		break;
	}
}

void Camera::setZoom(int zoomNumber) {
	currentZoom = zoomNumber;
	setView(currentView);
}

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(position, target, up);
}