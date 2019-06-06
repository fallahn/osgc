///header for physball class which interacts with both vehicles and map///

#ifndef PHYSBALL_H_
#define PHYSBAll_H_

#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>

#include <Game/PhysEntity.h>
#include <Game/Shaders/AlphaFalloff.h>

namespace Game
{
	class PhysBall : public PhysEntity
	{
	public:
		PhysBall(float radius = 20.f, sf::Color colour = sf::Color::Yellow);

		void Update(float dt, const sf::Vector2f& lightPos);
		//returns normalised radius for shader
		const float GetRadius() const
		{			
			return m_radius / static_cast<float>(m_renderTexture.getSize().x);
		};

	private:
		sf::CircleShape m_highlight, m_diffuse, m_shade, m_shadow;
		sf::RenderTexture m_renderTexture;
		sf::Color m_colour;
		float m_radius, m_shadowOffset;

		sf::Shader m_shader;
	};
};

#endif //physball