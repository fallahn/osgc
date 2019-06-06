////creates a lighting type energy fence between 2 points////
#ifndef ENERGY_FENCE_H_
#define ENERGY_FENCE_H_

#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/Clock.hpp>

#include <Game/AudioManager.h>
#include <Helpers.h>

#include <list>
#include <memory>

namespace Game
{
	class EnergyFence : public sf::Drawable, private sf::NonCopyable
	{
	public:
		EnergyFence(const sf::Vector2f& start, const sf::Vector2f& end, const sf::Texture& texture, AudioManager& audioManager);
		void Update(float dt);
		const sf::Vector2f GetStart() const{return m_start;};
		const sf::Vector2f GetEnd() const{return m_end;};
		void SetStart(const sf::Vector2f& point){m_start = point;};
		void SetEnd(const sf::Vector2f& point){m_end = point;};

	private:
		class Bolt;
		typedef std::unique_ptr<Bolt> BoltPtr;
		class Bolt : public sf::Drawable//series of vertices representing one bolt
		{
		public:
			enum State
			{
				Alive,
				Dead
			}CurrentState;
			Bolt() : CurrentState(Alive), m_transparency(255.f), m_vertices(sf::Quads){}
			void Update(float dt);
			static BoltPtr Create(const sf::Vector2f& start, const sf::Vector2f& end, const sf::Vector2f& texSize);
		private:
			struct Segment //for calulating points of a bolt
			{
				Segment(const sf::Vector2f& s, const sf::Vector2f& e)
					: start (s),
					end		(e){}
				sf::Vector2f start, end;
				sf::Vector2f Direction()const {return end - start;};
			};
			float m_transparency;
			sf::VertexArray m_vertices;

			void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		};
		sf::Vector2f m_start, m_end;
		const sf::Texture& m_texture;
		AudioManager& m_audioManager;
		sf::Sound m_electricLoop;
		float m_soundTime;
		sf::Clock m_soundTimer; // for firing random spark noises
		sf::Clock m_spawnClock;
		std::list< BoltPtr >m_bolts;
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}
#endif