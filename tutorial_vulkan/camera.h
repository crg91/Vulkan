#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<iostream>
#include<chrono>

typedef std::chrono::time_point<std::chrono::steady_clock> StdTime;

// This class is responsible for managing Camera properties.
//
class Camera
{
public:
	Camera();
	glm::mat4 getViewMatrix();
	void setMouseButtonPressed(bool fMouseButtonPressed);
	bool fMouseButtonPressed() { return m_fMousePressed; }
	void setInitialMousePosition(double xPos, double yPos) { m_lastMousePosition[0] = xPos; m_lastMousePosition[1] = yPos; }
	void setCurrentMousePosition(double xPos, double yPos) { m_currentMousePosition[0] = xPos; m_currentMousePosition[1] = yPos; }
private:
	double m_lastMousePosition[2];
	double m_currentMousePosition[2];
	bool m_fMousePressed;
	float m_radius;
	float m_theta;
	float m_phi;
};
