///creates an asteroid field and draws it via a vertex array///
#ifndef ASTEROIDS_H_
#define ASTEROIDS_H_

#include <Game/Vehicle.h>
#include <Game/ResourceManager.h>
#include <tmx/MapLoader.h>
#include <memory>
#include <array>

namespace Game
{
	class AsteroidField : public sf::Drawable, private sf::NonCopyable
	{
	public:
		AsteroidField(TextureResource& tr);
		void Update(float dt,
					const sf::Vector2f& lightPos,
					std::vector< std::unique_ptr<Vehicle> >& vehicles,
					tmx::MapLoader& map);
		void SetBounds(const sf::Vector2f& size);
	private:

		class Asteroid : private sf::NonCopyable
		{
		public:
			Asteroid(float size, float texSize);
			void Update(float dt, const sf::Vector2f& lightPos);
			void InvertVelocity(const sf::FloatRect& bounds);
			void InvertVelocity(const sf::Vector2f& normal);
			void Collide(Asteroid& other);
			void Collide(Vehicle& vehicle);
			void SetPosition(const sf::Vector2f& position);
			sf::Vector2f Position;
			std::array<sf::Vertex, 4> Vertices;
			std::array<sf::Vertex, 4> ShadowVertices;
			const float Radius;
			bool Tested; //mark if been collision tested or not
			bool InCollision; //prevent multiple collision reaction on a single collision
			sf::FloatRect GetGlobalBounds();
			sf::Vector2f GetPosition();
			sf::Vector2f GetLeadingEdge();
		private:
			sf::Transform m_transform;
			sf::Vector2f m_velocity, m_lastMovement;
			float m_mass;

			void m_Move(const sf::Vector2f& vel);
		};

		std::vector< std::unique_ptr<Asteroid> > m_asteroids;
		sf::FloatRect m_bounds;
		const sf::Uint8 m_roidCount;

		sf::RenderStates m_roidStates, m_shadowStates;
		sf::Texture& m_diffuse;
		sf::Texture& m_globeNormal;
		sf::Texture& m_craterNormal;
		sf::VertexArray m_roidVertices;
		sf::VertexArray m_shadowVertices;
		sf::Shader m_roidShader, m_shadowShader;
		void draw(sf::RenderTarget& target, sf::RenderStates states) const;
	};
};

#endif