#include "model.h"


/**************************************************************
* Description
*		Constructor for the class.
* Returns
*		void
* Notes
*
**************************************************************/
Model::Model()
{
}

/**************************************************************
* Description
*		Descriptor for the class.
* Returns
*		void
* Notes
*
**************************************************************/
Model::~Model()
{
}

/**************************************************************
* Description
*		updates the rotation around x axis based on the time
*		since last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateX()
{
	if (m_fKeyPressed[0])
	{
		if (m_fDirectionPositive[0])
		{
			m_durations.m_xDuration += getDuration(m_lastUpdateTime[0]);
		}
		else
		{
			m_durations.m_xDuration -= getDuration(m_lastUpdateTime[0]);
		}
	}
}

/**************************************************************
* Description
*		updates the rotation around y axis based on time since
*		last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateY()
{
	if (m_fKeyPressed[1])
	{
		if (m_fDirectionPositive[1])
		{
			m_durations.m_yDuration += getDuration(m_lastUpdateTime[1]);
		}
		else
		{
			m_durations.m_yDuration -= getDuration(m_lastUpdateTime[1]);
		}
	}
}

/**************************************************************
* Description
*		updates the rotation around z axis based on time elapsed
*		since last key stroke.
* Returns
*		void
* Notes
*
**************************************************************/
void Model::rotateZ()
{
	if (m_fKeyPressed[2])
	{
		if (m_fDirectionPositive[2])
		{
			m_durations.m_zDuration += getDuration(m_lastUpdateTime[2]);
		}
		else
		{
			m_durations.m_zDuration -= getDuration(m_lastUpdateTime[2]);
		}
	}
}

/**************************************************************
* Description
*		Returns the rotation matrix based based on the current state.
* Returns
*		void
* Notes
*
**************************************************************/
glm::mat4 Model::getModelMatrix()
{
	rotateX();
	rotateY();
	rotateZ();

	return
		glm::rotate(glm::mat4(1.0f), m_durations.m_xDuration * glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))*
		glm::rotate(glm::mat4(1.0f), m_durations.m_yDuration * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f))*
		glm::rotate(glm::mat4(1.0f), m_durations.m_zDuration * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

/**************************************************************
* Description
*		Gets the duration by looking at current time and last time.
* Returns
*		duration
* Notes
*
**************************************************************/
float Model::getDuration(StdTime &lastTime)
{
	auto currentTime = std::chrono::high_resolution_clock::now();
	float duration = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - lastTime).count();
	lastTime = currentTime;
	return duration;
}

/**************************************************************
* Description
*		Sets the state of the key for x axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setXKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[0] = fKeyPressed;
	m_lastUpdateTime[0] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the state of the key for y axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setYKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[1] = fKeyPressed;
	m_lastUpdateTime[1] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the state of the key for z axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::setZKeyPressed(bool fKeyPressed)
{
	m_fKeyPressed[2] = fKeyPressed;
	m_lastUpdateTime[2] = std::chrono::high_resolution_clock::now();
}

/**************************************************************
* Description
*		Sets the direction for the x axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fXDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[0] = fPositive;
}

/**************************************************************
* Description
*		Sets the direction for the y axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fYDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[1] = fPositive;
}

/**************************************************************
* Description
*		Sets the direction for the z axis rotation
* Returns
*		void
* Notes
*
**************************************************************/
void Model::fZDirectionPositive(bool fPositive)
{
	m_fDirectionPositive[2] = fPositive;
}