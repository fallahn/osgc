///source for material class///
#include <Mesh/Material.h>
#include <assert.h>

using namespace ml;

namespace
{
	const float defShininess = 80.f;
}

//ctor
Material::Material()
    :   m_ambientColour		(sf::Color(123u, 123u, 123u)),
        m_diffuseColour		(sf::Color(255u, 255u, 255u)),
        m_specularColour	(sf::Color(0u, 0u, 0u)),
		m_emissiveColour	(sf::Color(0u, 0u, 0u)),
		m_shininess			(defShininess),
        m_smooth			(true)
{

}

Material::Material(const sf::Color& ambient, const sf::Color& diffuse, const sf::Color& specular, const sf::Color& emissive)
	:	m_ambientColour		(ambient),
	m_diffuseColour			(diffuse),
	m_specularColour		(specular),
	m_emissiveColour		(emissive),
	m_shininess				(defShininess),
	m_smooth				(true)
{

}

//public
void Material::SetAmbientColour(const sf::Color& colour)
{
	m_ambientColour = colour;
}

const sf::Color& Material::GetAmbientColour() const
{
	return m_ambientColour;
}

void Material::SetDiffuseColour(const sf::Color& colour)
{
	m_diffuseColour = colour;
}

const sf::Color& Material::GetDiffuseColour() const
{
	return m_diffuseColour;
}

void Material::SetSpecularColour(const sf::Color& colour)
{
	m_specularColour = colour;
}

const sf::Color& Material::GetSpecularColour() const
{
	return m_specularColour;
}

void Material::SetEmissiveColour(const sf::Color& colour)
{
	m_emissiveColour = colour;
}

const sf::Color& Material::GetEmissiveColour() const
{
	return m_emissiveColour;
}

void Material::SetShininess(float shininess)
{
	assert(shininess >= 0.f && shininess <= 128.f);
	m_shininess = shininess;
}

float Material::GetShininess() const
{
	return m_shininess;
}

void Material::SetSmooth(bool b)
{
	m_smooth = b;
}

bool Material::Smooth() const
{
	return m_smooth;
}