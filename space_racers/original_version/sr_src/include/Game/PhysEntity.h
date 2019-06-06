///adds physics members to BaseEntity///
#ifndef PHYS_ENTITY_H_
#define PHYS_ENTITY_H_

#include <Game/BaseEntity.h>

namespace Game
{
	class PhysEntity : public BaseEntity
	{
	public:
		PhysEntity();

	protected:

		std::vector<sf::Vector2f> m_collisionPoints;
		float m_forceReduction; //amount force strength is reduced by over time
		float m_forceStrength; //amount ext force vector is multiplied by
		sf::Vector2f m_velocity, m_externalForce; //velocity and force to be applied to entity
		sf::Vector2f m_lastMovement; //total movement made by combining velocity and forces
		sf::Int8 m_collisionPoint; //ID of point which registered collision

		//adds an external force, eg from a collision
		void m_AddForce(const sf::Vector2f& direction, float strength);
		//returns true if polygon created by polypoints contains point
		bool m_Contains(const sf::Vector2f& point);
		//converts a collision point to world coordinates base on vehicle rotation / position
		const sf::Vector2f m_PointToWorld(const sf::Vector2f& point);
	private:



	};
};

#endif //PHYS_ENTITY_H_