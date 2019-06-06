///source code for 3D light class///
#include <Mesh/Light.h>
#include <Mesh/MeshHelpers.h>

using namespace ml;

//ctor
Light::Light()
    : m_ambientColour	(sf::Color(245u, 245u, 245u)),
      m_diffuseColour	(sf::Color(255u, 255u, 255u)),
      m_specularColour	(sf::Color(240, 240, 220)),
	  m_brightness		(0.f)
{

}

//public
void Light::SetPosition(const sf::Vector3f& position)
{
	m_position = position;
}

void Light::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

const sf::Vector3f& Light::GetPosition() const
{
	return m_position;
}

void Light::Move(const sf::Vector3f& amount)
{
	m_position += amount;
}

void Light::Move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Light::SetAmbientColour(const sf::Color& colour)
{
	m_ambientColour = colour;
}

const sf::Color& Light::GetAmbientColour() const
{
	return m_ambientColour;
}

void Light::SetDiffuseColour(const sf::Color& colour)
{
	m_diffuseColour = colour;
}

const sf::Color& Light::GetDiffuseColour() const
{
	return m_diffuseColour;
}

void Light::SetSpecularColour(const sf::Color& colour)
{
	m_specularColour = colour;
}

const sf::Color& Light::GetSpecularColour() const
{
	return m_specularColour;
}

void Light::CalcBrightness(const sf::Vector3f& nodePosition)
{
	sf::Vector3f distance = m_position - nodePosition;
	m_brightness = Helpers::Vectors::GetLength(distance);
}

float Light::GetBrightness() const
{
	return m_brightness;
}
//private