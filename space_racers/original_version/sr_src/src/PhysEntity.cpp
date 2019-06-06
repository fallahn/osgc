///source for phys entity functions///
#include <Game/PhysEntity.h>

using namespace Game;

PhysEntity::PhysEntity()
	: 	m_forceStrength		(0.f),
	m_collisionPoint	(-1)
{

}


//public



//private
void PhysEntity::m_AddForce(const sf::Vector2f& direction, float strength)
{
	m_forceStrength = strength;
	m_externalForce = Helpers::Vectors::Normalize(direction);
}

bool PhysEntity::m_Contains(const sf::Vector2f& point)
{
	unsigned int i, j;
	bool result = false;
	for (i = 0, j = m_collisionPoints.size() - 1; i < m_collisionPoints.size(); j = i++)
	{
		//convert to world coord
		sf::Vector2f vertA = m_PointToWorld(m_collisionPoints[i]);
		sf::Vector2f vertB = m_PointToWorld(m_collisionPoints[j]);

		if(((vertA.y > point.y) != (vertB.y > point.y)) &&
			(point.x < (vertB.x - vertA.x) * (point.y - vertA.y)
				/ (vertB.y - vertA.y) + vertA.x))
		result = !result;
	}
	return result;
}

const sf::Vector2f PhysEntity::m_PointToWorld(const sf::Vector2f& point)
{
	sf::Transform tf;
	tf.rotate(m_sprite.getRotation(), m_sprite.getPosition());
	
	return tf.transformPoint(m_sprite.getPosition() + point);
}