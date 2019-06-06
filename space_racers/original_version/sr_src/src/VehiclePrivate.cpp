//***definitions of private functions of car class***//
#include <Game/Vehicle.h>
#include <Game/SharedData.h>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Joystick.hpp>

using namespace Game;

void Vehicle::m_Accelerate(float dt)
{
	m_UpdateVelocity();

	if(!(m_controlMask & HANDBRAKE) && m_currentSpeed < m_topSpeed)
		m_currentSpeed += m_acceleration;
	else if((m_controlMask & HANDBRAKE) && m_currentSpeed > (m_topSpeed / 2.f)) //don't reduce to 0
		m_currentSpeed -= (m_handbrakeAmount * dt); //reduce speed
}

void Vehicle::m_Brake(float dt)
{
	m_UpdateVelocity();

	if(!(m_controlMask & HANDBRAKE) && m_currentSpeed > -(m_topSpeed * 0.7f))
		m_currentSpeed -= m_deceleration;
	else if((m_controlMask & HANDBRAKE) && m_currentSpeed < -(m_topSpeed * 0.3f))
		m_currentSpeed += (m_handbrakeAmount * dt);
			
}

void Vehicle::m_Rotate(float angle)
{
	//don't steer while stationary
	if(m_currentSpeed == 0.f && m_forceStrength == 0.f) return;
	//otherwise reduce steering with speed
	if(!(m_controlMask & HANDBRAKE) && m_currentSpeed > (m_topSpeed / 1.2f))
		angle *= ((m_topSpeed * 2.5f) / m_currentSpeed);
	else //or tighter turns when handbraking
		angle *= 2.8f;
	
	//invert angle when reversing
	if(m_currentSpeed < 0.f) angle =- angle;

	//rotate sprite
	m_sprite.rotate(angle);

}

const sf::Vector2f Vehicle::m_GetNormal(const sf::Vector2f& direction) const
{
	sf::Transform tf;
	tf.rotate(m_sprite.getRotation());
	return tf.transformPoint(direction);
}

void Vehicle::m_UpdateVelocity()
{
	if((m_controlMask & HANDBRAKE) || m_zHeight || m_state != Alive) return;
	sf::Vector2f oldVel = m_velocity;
	m_velocity = m_GetNormal(FORWARD_VECTOR);			
	//reduce speed by any rotation to give 'wheelspin' effect
	m_currentSpeed *= Helpers::Vectors::Dot(oldVel, m_velocity);
}

void Vehicle::m_ParseInput()
{
	m_controlMask = 0u;

	switch(m_controlSet)
	{
	default:
	case KeySetOne:
		//cursors, right control
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			m_controlMask |= ACCELERATE;
		else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			m_controlMask |= DECELERATE;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			m_controlMask |= LEFT;
		else if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			m_controlMask |= RIGHT;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::RControl))
			m_controlMask |= HANDBRAKE;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))
			m_controlMask |= TURBO;

		break;
	case KeySetTwo:
		//WASD, space
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::W))
			m_controlMask |= ACCELERATE;
		else if(sf::Keyboard::isKeyPressed(sf::Keyboard::S))
			m_controlMask |= DECELERATE;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::A))
			m_controlMask |= LEFT;
		else if(sf::Keyboard::isKeyPressed(sf::Keyboard::D))
			m_controlMask |= RIGHT;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space))
			m_controlMask |= HANDBRAKE;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
			m_controlMask |= TURBO;

		break;
	case ControllerOne:
		m_ParseController(0);
		break;
	case ControllerTwo:
		m_ParseController(1);
		break;
	case Network:
		//TODO send controller state if player local, including
		//controller X and Z values

		break;
	case CPU:
		Ai.Update();
		break;
	}
}

void Vehicle::m_ParseController(sf::Uint8 id)
{
	float stickThresh = 20.f;
	float triggerThresh = 0.2f;
	float x, z;
	
	//use controller analogue values to adjust top speed and turn speed
	z = sf::Joystick::getAxisPosition(id, sf::Joystick::Z);
	if(z < -triggerThresh)
	{
		m_controlMask |= ACCELERATE;
		//only increase speed so letting go of throttle decelerates normally
		float newSpeed = m_defTopSpeed * (-z / 100.f);
		if(newSpeed > m_topSpeed) m_topSpeed = newSpeed;
	}
	else if(z > triggerThresh)
	{
		m_controlMask |= DECELERATE;
		/*float newSpeed = m_defTopSpeed * (z / 100.f);
		if(newSpeed < m_topSpeed) m_topSpeed = newSpeed;*/
	}

	x = sf::Joystick::getAxisPosition(id, sf::Joystick::X);
	if(x > stickThresh)
	{
		m_controlMask |= LEFT;
		m_turnSpeed = m_defTurnSpeed * (-x / 100.f);
	}
	else if(x < -stickThresh)
	{
		m_controlMask |= RIGHT;
		m_turnSpeed = m_defTurnSpeed * (x / 100.f);
	}

	//check POV secondly to override pressing analogue too
	const float povThresh = 10.f;
	x = sf::Joystick::getAxisPosition(id, sf::Joystick::PovX);
	if(x < -povThresh)
	{
		m_controlMask |= LEFT;
		m_turnSpeed = m_defTurnSpeed;
	}
	else if(x > povThresh)
	{
		m_controlMask |= RIGHT;
		m_turnSpeed = m_defTurnSpeed;
	}


	if(sf::Joystick::isButtonPressed(id, 0)) m_controlMask |= TURBO;
	if(sf::Joystick::isButtonPressed(id, 1)) m_controlMask |= HANDBRAKE;
}

void Vehicle::m_UpdateCollision(float dt, std::vector<tmx::MapObject*>& objects)
{
	//reset the last collision point
	m_collisionPoint = -1;
	
	//iterate through objects and deal with accordingly
	for(auto currentObject = objects.begin(); currentObject != objects.end(); ++currentObject)
	{
		tmx::MapObject& object = **currentObject;
		std::string parent = object.GetParent();
		if(parent.empty()) return;
		else if(parent == "Waypoints" && object.Contains(m_sprite.getPosition()))
		{
			sf::Uint8 id = std::stoi(object.GetName());
			if(id != m_currentNode.ID) m_suggestNode = true;
		}

		sf::Uint8 pointId = 0u;
		for(auto point = m_collisionPoints.begin(); point != m_collisionPoints.end(); ++point)
		{	
			if(object.Contains(m_PointToWorld(*point)))
			{
				if(parent == "Collision" && !m_inCollision)
				{
					//get collision normal and reflect velocity about it
					sf::Vector2f worldPoint = m_PointToWorld(*point);
					sf::Vector2f collisionNormal = object.CollisionNormal(worldPoint - m_lastMovement, worldPoint);
					
					sf::Vector2f offset = Helpers::Vectors::Normalize(Helpers::Vectors::Reflect(m_lastMovement, collisionNormal));
					if(Helpers::Vectors::GetLength(m_lastMovement) != 0)
					{
						while(object.Contains(m_PointToWorld(*point)))
							m_sprite.move(-m_lastMovement * dt); //move back to where we were before collision
					}
					m_sprite.move(offset * m_defTopSpeed * dt);

					//rebound from object and kill speed
					m_AddForce(/*-m_velocity*/offset, m_currentSpeed * 0.4f); 
					m_health -= 350.f * dt;
					m_currentSpeed *= 0.9f;

					//update id of collision point
					m_collisionPoint = pointId;//std::distance(m_collisionPoints.begin(), point);
					m_PlayImpactSound();
					m_inCollision = true;
				}
				else if(parent == "Jump")
				{
					m_Jump(0.28f); //TODO get jump height from map property if available
				}
				else if(parent == "Waypoints")
				{
					std::string name = object.GetName();
					if(name.empty()) continue;
					try
					{
						//get id
						sf::Uint8 id = std::stoi(name);
						
						//skip reassigning if we are retesting same node
						if(id == m_currentNode.ID) continue;

						//else test for short cutting
						sf::Uint8 nextId, lastId;
						if(m_currentNode.ID == 0) lastId = m_nodes.size() - 1;
						else lastId = m_currentNode.ID - 1;
						
						if(m_currentNode.ID == m_nodes.size() - 1) nextId = 0;
						else nextId = m_currentNode.ID + 1;

						if(id != nextId && id != lastId) Kill(); //took a short cut
					}
					catch(std::invalid_argument& ex)
					{
						std::cerr << "Hit invalid waypoint " << ex.what() << std::endl;
					}
				}
				else if(parent == "Friction")
				{
					std::string name = object.GetName();
					if(name.empty()) continue;
					try
					{
						float value = std::stof(name);
						//slow down but don't speed up
						if(value > 1.f) m_currentSpeed *= (1.f / value);

						//increase drift when decreasing friction
						m_driftAmount *= (1.f / value);
						if(m_driftAmount > 1.f) m_driftAmount = 1.f;

						//and adjust deceleration;
						m_deceleration *= value;

						//and turn speed
						m_turnSpeed *= (1.f / value);
					}
					catch(std::invalid_argument& ex)
					{
						std::cerr << "Hit friction object with invalid value " << ex.what() << std::endl;
					}
				}
				else if(parent == "KillZone")
				{
					//TODO we can't eliminate in time trial mode
					Kill(/*Eliminated*/);
				}
				//TODO other object types
				break; //skip other points for this object
			}
			else if(*point == m_collisionPoints.back())
			{
				//we have the last point and no collision
				m_inCollision = false;
			}
			pointId++;
		}

		//check if we fall off the edge
		if(m_type != Ship 
			&& parent == "Space" 
			&& object.Contains(m_sprite.getPosition()) 
			&& m_zHeight == 0.f)
				m_state = Falling;
	}
}

void Vehicle::m_UpdatePosition(float dt)
{
	//coast to a halt
	if(m_currentSpeed > m_deceleration)
		m_currentSpeed -= m_deceleration;
	else if(m_currentSpeed < -m_deceleration / 2.f)
		m_currentSpeed += m_deceleration / 2.f;
	else m_currentSpeed = 0.f;

	//reduce external force (allowing for negative forces)
	if(m_forceStrength > m_forceReduction)
		m_forceStrength -= m_forceReduction;
	else if(m_forceStrength < -m_forceReduction)
		m_forceStrength += m_forceReduction;
	else m_forceStrength = 0.f;

	//apply handbrake to external force
	if(m_controlMask & HANDBRAKE)		
		if(m_forceStrength > 0.8f)
			m_forceStrength -= (m_handbrakeAmount * dt);
		else if(m_forceStrength < -0.8f)
			m_forceStrength += (m_handbrakeAmount * dt);
	
	//check if vehicle is off the ground
	if(m_type != Ship) //ships don't jump
	{
		m_zHeight += m_jumpPower; //TODO multiply jump power by dt
		m_jumpPower -= (1.8f * dt);
		if(m_zHeight < 0.f)
		{
			m_zHeight = 0.f;
			m_jumpPower = 0.f;
		}
	}
	
	//calc drift
	//drift for normal movement	
	if(m_type != Ship && m_zHeight) m_driftAmount = 0.97f;
	else if((m_controlMask & HANDBRAKE) || m_state == Falling) m_driftAmount = 1.f;

	m_frontNormal = m_GetNormal(FORWARD_VECTOR);
	m_sideNormal = m_GetNormal(SIDE_VECTOR);

	sf::Vector2f forwardVelocity = m_frontNormal * Helpers::Vectors::Dot(m_velocity, m_frontNormal);
	sf::Vector2f sideVelocity = m_sideNormal * Helpers::Vectors::Dot(m_velocity, m_sideNormal);
	m_velocity = forwardVelocity  + (sideVelocity * m_driftAmount);//multiply between 0-1 depending on amount of lateral movement wanted

	//drift for external forces applied
	forwardVelocity = m_frontNormal * Helpers::Vectors::Dot(m_externalForce, m_frontNormal);
	sideVelocity = m_sideNormal * Helpers::Vectors::Dot(m_externalForce, m_sideNormal);
	m_externalForce = forwardVelocity + sideVelocity * m_driftAmount; //NEVER make this more than 1.0!!!

	//return drift amount in case it has been modified elsewhere
	m_driftAmount = m_defDriftAmount;

	//move vehicle
	m_lastMovement = (m_velocity * m_currentSpeed) + (m_externalForce * m_forceStrength);
	m_sprite.move(m_lastMovement * dt);
}

void Vehicle::m_UpdateTexture()
{
	//transform light position
	sf::Vector2f lightPos = m_material.lightPosition - m_sprite.getPosition();
	sf::Transform tf;
	tf.rotate(-m_sprite.getRotation());
	lightPos = tf.transformPoint(lightPos);
	//update shadow from light position
	m_shadowSprite.setPosition((m_material.textureSize / 2.f) - (Helpers::Vectors::Normalize(lightPos) * m_shadowOffset));

	//darken sprite based on distance from light
	//if(m_state == Alive)
	//{
	//	float distance = Helpers::Vectors::GetLength(lightPos);
	//	distance /= m_material.lightPosition.y;
	//	distance = 1.f - distance;
	//	distance *= 108.f;
	//	sf::Uint8 lightVal = 147u + static_cast<sf::Uint8>(distance);
	//	m_sprite.setColor(sf::Color(lightVal, lightVal, lightVal));
	//}

	//add jumping effect
	sf::Vector2f offset = Helpers::Vectors::Normalize(sf::Vector2f(1.f, -1.f)) * std::min(m_shadowOffset, m_zHeight);
	m_baseSprite.setPosition((m_material.textureSize / 2.f) + offset);


	//sf::VertexArray arr;
	//for(auto p = m_collisionPoints.begin(); p != m_collisionPoints.end(); ++p)
	//	arr.append(sf::Vertex(*p + m_sprite.getOrigin(), sf::Color::Magenta));
	//arr.append(sf::Vertex(m_material.textureSize / 2.f, sf::Color::Green));
	//arr.append(sf::Vertex(lightPos, sf::Color::Green));
	//arr.setPrimitiveType(sf::LinesStrip);

	//transform cam position
	//sf::Vector2f camPos = m_material.cameraPosition - m_sprite.getPosition();
	//camPos = tf.transformPoint(camPos);

	//update shader properties
	m_material.normalShader->setParameter("lightPosition", sf::Vector3f(lightPos.x, lightPos.y, m_lightZ));
	//m_material.shader->setParameter("cameraPosition", camPos);
	m_material.normalShader->setParameter("reflectOffset", m_sprite.getPosition() * 0.5f);
	m_material.normalShader->setParameter("neonColour", PlayerColours::Light(m_playerId));

	//render to texture
	m_renderTexture.clear(sf::Color::Transparent);
	
	switch(m_state)
	{
	case Alive:
	default:
		m_renderTexture.draw(m_shadowSprite);
		m_renderTexture.draw(m_baseSprite, m_material.normalShader.get());
		break;
	case Falling:
		m_renderTexture.draw(m_baseSprite, m_material.normalShader.get());
		break;
	case Dying:
		
		break;
	case Dead:

		break;
	case Eliminated:
		
		break;
	case Celebrating:
		m_renderTexture.draw(m_shadowSprite);
		m_renderTexture.draw(m_baseSprite, m_material.normalShader.get());
		break;
	}
	
	//m_renderTexture.draw(arr);
	m_renderTexture.display();
}

void Vehicle::m_Jump(float jumpPower)
{
	if(jumpPower > 1.f) jumpPower = 1.f;
	if(m_jumpPower == 0.f) m_jumpPower = jumpPower * (m_currentSpeed / m_defTopSpeed);
}

void Vehicle::m_SeparateVehicle(float dt, Vehicle& vehicle)
{			
	float force = Helpers::Vectors::GetLength(m_lastMovement) * 0.55f;
	vehicle.m_AddForce(m_lastMovement, force); //repel other car
	m_AddForce(-m_velocity, m_currentSpeed * 0.8f); //kill own velocity

	//split cars apart by moving them
	sf::Vector2f direction = Helpers::Vectors::Normalize(m_sprite.getPosition() - vehicle.GetCentre());
	m_sprite.move(direction * m_currentSpeed * 1.5f * dt);
	vehicle.m_sprite.move(-direction * m_defTopSpeed * dt);

	//do some damage
	m_health -= 250.f * dt;
	vehicle.m_health -=  250.f *dt;
	m_PlayImpactSound();
}

void Vehicle::m_StartParticles()
{
	m_smokeSystem.Start();
	if(m_type != Ship) m_tyreSystem.Start();
}

void Vehicle::m_StopParticles()
{
	m_smokeSystem.Stop();
	m_tyreSystem.Stop();
}

void Vehicle::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	rt.draw(m_sprite);
	if(m_explosionSprite.playing())
		rt.draw(m_explosionSprite, sf::BlendAdd);
}