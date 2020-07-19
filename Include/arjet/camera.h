#pragma once
//adapted from learnopengl

#include <arjet/renderer.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>


using glm::vec3;
using glm::vec4;
using glm::mat4;

enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT
};

//defaults
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 2.5f;
const float SENSITIVITY = 0.05f;
const float ZOOM = 45.0f;

class Camera : public Component{
public:
	

	float MovementSpeed;
	float MouseSensitivity;
	float Zoom;

	Camera() : Front(vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
		WorldUp = vec3(0.0f, 1.0f, 0.0f);
		Yaw = nullptr;
		Pitch = nullptr;
	}

	mat4 GetViewMatrix();

	void start();

	void ProcessKeyboard(Camera_Movement direction, float deltaTime);

	void ProcessMouseMovement(float xoffset, float yoffset, uint32_t constrainPitch = 1);

	// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset);

private:
	vec3* Position;
	vec3 Front;
	vec3 Up;
	vec3 Right;
	vec3 WorldUp;

	float* Yaw;
	float* Pitch;

	void updateCameraVectors();
};