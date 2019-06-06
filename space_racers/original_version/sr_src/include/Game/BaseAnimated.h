/*///header file for base class for animated sprites such as players an enemies///
This class is derived from BaseEntity and is designed to create objects which
use a default sprite sheet / set of animation ranges. Classes such as players 
and NPCs are all derived from this */
#ifndef _BASEANIMATED_H_
#define _BASEANIMATED_H_

#include <Game/BaseEntity.h>

#define RUN 0, 10
#define IDLE 11, 20

namespace Game
{
	class BaseAnimated : public BaseEntity
	{
	public:
		BaseAnimated(sf::Vector2f position = sf::Vector2f(),
			sf::Texture* texture = 0, sf::Uint16 frameCountHor = 1u, sf::Uint16 frameCountVert = 1u);
		virtual ~BaseAnimated(){};
		//performs the default update routine on the entity. Should be called at least once per frame
		//should be called by any overriding functions in derived classes
		virtual void Update(float dt);
		//gets the current velocity of the entity
		const sf::Vector2f& GetVelocity(void);
		//sets the velocity of the entity
		void SetVelocity(sf::Vector2f& velocity);

	protected:
		sf::Clock m_clock;
		Direction m_direction;
		sf::Vector2f m_velocity;
		float m_moveSpeed; //value by which the normalised velocity is multiplied by

		//default animation functions
		virtual void m_Run(void);
		virtual void m_Idle(void);
		//virtual void m_Reset(sf::Vector2f position) = 0;
	};
};


#endif