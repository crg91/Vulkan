#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include<iostream>
#include<chrono>

typedef std::chrono::time_point<std::chrono::steady_clock> StdTime;

// We keep this structure to keep track of durations for
// rotations in each axis.
// TODO : Consider using glm for this.
//
struct DurationForRotation
{
	float m_xDuration;
	float m_yDuration;
	float m_zDuration;
};

class Rotation
{
public:
	void rotateX();
	void rotateY();
	void rotateZ();
	float getDuration(StdTime &lastTime);
	glm::mat4 rotationMatrix();
	void setXKeyPressed(bool fKeyPressed);
	void fXDirectionPositive(bool fPositive);
	void setYKeyPressed(bool fKeyPressed);
	void fYDirectionPositive(bool fPositive);
	void setZKeyPressed(bool fKeyPressed);
	void fZDirectionPositive(bool fPositive);
private:
	DurationForRotation m_durations;
	StdTime m_lastUpdateTime[3];
	bool m_fKeyPressed[3];
	bool m_fDirectionPositive[3];
};
