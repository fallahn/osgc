///***source for AI class of vehicles***//
#include <Game/Vehicle.h>

using namespace Game;

//ctor



//dtor



//public functions
void Vehicle::Ai::Update()
{	
	//check we are allowed to move
	if(m_delayClock.getElapsedTime().asSeconds() < m_delayTime) return;

	m_parent.m_acceleration *= 0.98f;

	//update behaviour occasionally with random values
	if(m_alterClock.getElapsedTime().asSeconds() > m_alterTimeout)
	{
		m_alterClock.restart();
		m_brakingDistance = Helpers::Random::Float(165.f, 200.f);
	}

	m_parent.m_controlMask |= m_parent.ACCELERATE;
	
	m_parent.m_topSpeed = m_parent.m_defTopSpeed * m_topSpeedMultiplier;
	
	//steer away from collisions
	if(m_parent.m_collisionPoint == 0 ||
		m_parent.m_collisionPoint == 1)
	{
		m_parent.m_controlMask |= m_parent.LEFT;
	}
	else if(m_parent.m_collisionPoint == 2 ||
		m_parent.m_collisionPoint == 3)
	{
		m_parent.m_controlMask |= m_parent.RIGHT;
	}

	//TODO avoid the edge

	//if not avoiding collision turn towards next node
	if(m_parent.m_collisionPoint == -1)
	{
		//get angle between current direction and direction to next node
		//return next node to world coords and work out relative to car pos
		sf::Vector2f newDir = (m_parent.m_currentNode.NextUnit * m_parent.m_currentNode.NextMagnitude) + m_parent.m_currentNode.Position;
		newDir = m_parent.m_sprite.getPosition() - newDir;

		float angle = Helpers::Vectors::GetAngle(newDir);
		if(angle < 0.f) angle += 360.f;

		angle -= m_parent.m_sprite.getRotation();
		//invert mis-calcs
		if(angle > 180.f) angle -= 360.f;
		else if(angle < -180.f) angle += 360.f;

		//std::cerr << angle << std::endl;

		if(angle > 2.f) //TODO move this to own function
		{
			if(Helpers::Vectors::GetLength(newDir) < m_brakingDistance
				&& m_parent.m_currentSpeed  > m_parent.m_topSpeed * 0.98f)
			{
				m_parent.m_controlMask |= m_parent.DECELERATE;
				m_parent.m_controlMask &= ~m_parent.ACCELERATE;
			}
			
			//slow down for wide angles
			if(angle > 38.f  && m_parent.m_currentSpeed  > m_parent.m_topSpeed * 0.75f)
				m_parent.m_controlMask |= m_parent.HANDBRAKE;

			m_parent.m_controlMask |= m_parent.RIGHT;
		}
		else if(angle < -2.f)
		{
			if(Helpers::Vectors::GetLength(newDir) < m_brakingDistance
				&& m_parent.m_currentSpeed  > m_parent.m_topSpeed * 0.98f)
			{
				m_parent.m_controlMask |= m_parent.DECELERATE;
				m_parent.m_controlMask &= ~m_parent.ACCELERATE;
			}

			if(angle < -38.f && m_parent.m_currentSpeed  > m_parent.m_topSpeed * 0.75f)
				m_parent.m_controlMask |= m_parent.HANDBRAKE;

			m_parent.m_controlMask |= m_parent.LEFT;
		}
	}
}


//private function