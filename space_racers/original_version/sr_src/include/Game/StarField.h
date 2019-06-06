//basic particle style starfield for menu background//
#ifndef STARFIELD_H_
#define STARFIELD_H_

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/System/Clock.hpp>

#include <Game/Common.h>
#include <Game/ResourceManager.h>
#include <Helpers.h>

#include <memory>
#include <array>

namespace Game
{
	class StarField : public sf::Drawable
	{
	public:
		StarField(TextureResource& textures);
		//updates with own calculated velocity
		void Update(float dt, const sf::FloatRect& viewArea);
		//updates with custom velocity
		void Update(float dt, const sf::FloatRect& viewArea, const sf::Vector2f& velocity);

	private:
		struct Star
		{
			Star(float minSpeed = 200.f, float maxSpeed = 800.f);
			void Update(float dt, const sf::Vector2f& velocity);
			sf::RectangleShape Shape;
			sf::Color Colour;

		private:
			const float m_minSpeed;
			const float m_maxSpeed;
			const float m_minSize;
			const float m_maxSize;
			float m_speed, m_size;
			bool m_grow;
			void m_Reset();
		};

		std::vector< std::shared_ptr<Star> > m_nearStars;
		std::vector< std::shared_ptr<Star> > m_farStars;
		sf::Vector2f m_velocity, m_lastVelocity;
		sf::Clock m_randClock;

		float m_acceleration, m_randTime;

		//nebula background
		sf::Texture m_nebulaTexture;
		sf::Sprite m_nebulaSprite;

		//planet in background
		sf::Texture m_planetDiffuse, m_planetCraterNormal, m_planetGlobeNormal;
		sf::Sprite m_planetSprite;
		sf::Shader m_planetShader;
		sf::Vector2f m_planetResolution, m_planetPosition;


		//star texture
		sf::Texture m_starTexture;
		std::array<sf::Vector2f, 4> m_texCoords;

		void m_boundStar(std::shared_ptr<Star>& star, const sf::FloatRect& viewArea);
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
};


#endif //STARFIELD_H_