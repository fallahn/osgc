///source for camera class
#include <Mesh/Camera.h>
#include <Mesh/GL/glew.h>
#include <assert.h>

using namespace ml;

//ctor / dtor
Camera::Camera()
	:	m_FOV		(22.5f),
	m_nearValue		(150.f),
	m_farValue		(1400.f),
	m_aspectRatio	(16.f / 9.f)
{

}

Camera::~Camera()
{

}

//public
void Camera::SetPosition(const sf::Vector3f& position)
{
	m_position = position;
}

void Camera::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

const sf::Vector3f& Camera::GetPosition() const
{
	return m_position;
}

void Camera::Move(const sf::Vector3f& amount)
{
	m_position += amount;
}

void Camera::Move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Camera::SetRotation(const sf::Vector3f& rotation)
{
	m_rotation = rotation;
}

void Camera::SetRotation(float x, float y, float z)
{
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;
}

const sf::Vector3f& Camera::GetRotation() const
{
	return m_rotation;
}

void Camera::Rotate(const sf::Vector3f& amount)
{
	m_rotation += amount;
	m_ClampRotation();
}

void Camera::Rotate(float x, float y, float z)
{
	m_rotation.x += x;
	m_rotation.y += y;
	m_rotation.z += z;
	m_ClampRotation();
}

void Camera::SetFOV(float angle)
{
	assert(angle < 180.f);
	m_FOV = angle;
}

float Camera::GetFOV() const
{
	return m_FOV;
}

void Camera::SetAspectRatio(float ratio)
{
	assert(ratio > 0.f);
	m_aspectRatio = ratio;
}

float Camera::GetAspectRatio() const
{
	return m_aspectRatio;
}

void Camera::SetNearValue(float distance)
{
	assert(distance < m_farValue);
	m_nearValue = distance;
}

float Camera::GetNearValue() const
{
	return m_nearValue;
}

void Camera::SetFarValue(float distance)
{
	assert(distance > m_nearValue);
	m_farValue = distance;
}

float Camera::GetFarValue() const
{
	return m_farValue;
}

void Camera::Update(float dt)
{
	//override this in derived classes
}

void Camera::Draw()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(m_FOV, m_aspectRatio, m_nearValue, m_farValue);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(m_rotation.x, 1, 0, 0);
    glRotatef(m_rotation.y, 0, 1, 0);
    glRotatef(m_rotation.z, 0, 0, 1);
    glTranslatef(m_position.x, m_position.y, m_position.z);
}

///private
void Camera::m_ClampRotation()
{
	if(m_rotation.x > 360.f) m_rotation.x -= 360.f;
	else if (m_rotation.x < -360.f) m_rotation.x += 360.f;

	if(m_rotation.y > 360.f) m_rotation.y -= 360.f;
	else if (m_rotation.y < -360.f) m_rotation.y += 360.f;

	if(m_rotation.z > 360.f) m_rotation.z -= 360.f;
	else if (m_rotation.z < -360.f) m_rotation.z += 360.f;
}