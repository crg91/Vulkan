#include "camera.h"

/**************************************************************
* Description
*		Helper function to convert spherical coordinates to 
*		planar coordinates.
* Returns
*		void
* Notes
*
**************************************************************/
glm::vec3 sphericalToNormal(float radius, float theta, float phi)
{
	return glm::vec3(
		radius * glm::cos(glm::radians(phi)),
		radius * glm::sin(glm::radians(phi)) * glm::sin(glm::radians(theta)),
		radius * glm::sin(glm::radians(phi)) * glm::cos(glm::radians(theta))
	);
}

/**************************************************************
* Description
*		Constructor.
* Returns
*		void
* Notes
*
**************************************************************/
Camera::Camera():m_fMousePressed(false)
{
	m_phi = 90.0f;
	m_theta = 0.0f;
	m_radius = 5.0f;
}

/**************************************************************
* Description
*		Returns the rotation matrix based based on the current state.
* Returns
*		void
* Notes
*
**************************************************************/
glm::mat4 Camera::getViewMatrix()
{
	float modifiedTheta = m_theta;
	float modifiedPhi = m_phi;

	if (m_fMousePressed)
	{
		modifiedTheta += (m_currentMousePosition[1] - m_lastMousePosition[1]) / 10.0f;
		modifiedPhi += (m_currentMousePosition[0] - m_lastMousePosition[0]) / 10.0f;
	}

	glm::vec3 eyeVector = sphericalToNormal(m_radius, modifiedTheta, modifiedPhi);
	glm::vec3 rightVector = glm::vec3(
		m_radius * glm::sin(glm::radians(modifiedPhi)),
		0.0,
		-m_radius * glm::cos(glm::radians(modifiedPhi)));
	glm::vec3 upVector = glm::cross(eyeVector, rightVector);
	return glm::lookAt(eyeVector, glm::vec3(0.0f, 0.0f, 0.0f), upVector);
}

/**************************************************************
* Description
*		Sets the mouse button press state. We also change other
*		states based on that.
* Returns
*		void
* Notes
*
**************************************************************/
void Camera::setMouseButtonPressed(bool fMouseButtonPressed)
{
	if (!fMouseButtonPressed)
	{
		m_theta += (m_currentMousePosition[1] - m_lastMousePosition[1]) / 10.0f;
		m_phi += (m_currentMousePosition[0] - m_lastMousePosition[0]) / 10.0f;
	}
	m_fMousePressed = fMouseButtonPressed;
}