////simple class to hold opengl material properties////
#ifndef MATERIAL_H_
#define MATERIAL_H_

#include <SFML/Graphics/Color.hpp>

namespace ml
{
	class Material final
	{
	public:
		Material();
		Material(const sf::Color& ambient, const sf::Color& diffuse, const sf::Color& specular, const sf::Color& emissive);

		void SetAmbientColour(const sf::Color& colour);
		const sf::Color& GetAmbientColour() const;
		void SetDiffuseColour(const sf::Color& colour);
		const sf::Color& GetDiffuseColour() const;
		void SetSpecularColour(const sf::Color& colour);
		const sf::Color& GetSpecularColour()const;
		void SetEmissiveColour(const sf::Color& colour);
		const sf::Color& GetEmissiveColour() const;
		void SetShininess(float amount);
		float GetShininess() const;
		void SetSmooth(bool b);
		bool Smooth() const;

	private:
		sf::Color m_ambientColour, m_diffuseColour;
		sf::Color m_specularColour, m_emissiveColour;
		float m_shininess;
		bool m_smooth;
	};
}

#endif //MATERIAL_H_
