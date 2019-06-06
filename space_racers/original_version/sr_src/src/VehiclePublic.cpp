//***definitions for public functions of car class***//

#include <Game/Vehicle.h>
//#include <Game/Console.h>

using namespace Game;

//ctor
Vehicle::Vehicle(Config& config, const std::vector<TrackNode>& nodes, sf::Uint8 playerId, ControlSet controls, Intelligence intelligence)
	//---------consts---------//
	: m_defAcceleration	(config.acceleration),
	m_defDeceleration	(config.deceleration),
	m_defTopSpeed		(config.topSpeed),
	m_defTurnSpeed		(config.turnSpeed),  //number of degrees to rotate per second when turning
	m_defForceReduction	(2200.f),//amount external force is reduced by per second
	m_handbrakeAmount	(config.handbrakeAmount),  //amount of handbraking applied per second
	m_maxHealth			(config.maxHealth),
	m_mass				(config.mass),
	m_healthIncrease	(300.f), //rate at which health regenerates per second
	FORWARD_VECTOR		(sf::Vector2f(-1.f, 0.f)), //used in rotation and inertia calc
	SIDE_VECTOR			(sf::Vector2f(0.f, 1.f)),
	//-------end consts-------//
	m_currentSpeed		(0.f),
	m_topSpeed			(m_defTopSpeed),
	m_turnSpeed			(m_defTurnSpeed),
	m_zHeight			(0.f),  //current height of vehicle above ground
	m_jumpPower			(0.f),  //current jump velocity
	m_spriteScale		(1.f),
	m_shadowOffset		(config.material.shadowOffset), //maximum distance in pixels vehicle shadow is offset by
	m_lightZ			(2.75f),
	m_health			(m_maxHealth),
	m_defDriftAmount	(config.defDriftAmount),
	m_driftAmount		(config.driftAmount),
	m_controlSet		(controls),
	m_controlMask		(0u),
	m_lapDistance		(0.f),
	m_nodeDistance		(0.f),
	m_accumulatedDistance(0.f),
	m_spawnPosition		(static_cast<float>(playerId)),
	m_lapLength			(0.f),
	//-----input consts-----//
	ACCELERATE			(0x01),
	DECELERATE			(0x02),
	LEFT				(0x04),
	RIGHT				(0x08),
	HANDBRAKE			(0x10),
	TURBO				(0x20),
	//-----------------------//
	m_explosionSprite	(config.material.explosion, 256, 256),
	m_type				(config.type),
	m_material			(config.material),
	m_racePosition		(playerId + 1u),
	m_currentNode		(*nodes.begin()),
	m_currentLap		(1u),
	m_playerId			(playerId),
	m_colour			(255.f),
	m_inputEnabled		(false),
	m_invincible		(true),
	m_invincibleTime	(3.f),
	m_suggestNode		(false),
	m_impactLimit		(0.1f), //amount of time before an impact registers more damage
	m_inCollision		(false),
	Ai					(*this, intelligence),
	m_smokeSystem		((m_type == Buggy) ? 60.f : 30.f, 30u, SmokeParticle::Create),
	m_tyreSystem		((m_type == Buggy) ? 100.f : 60.f, 60u, SkidParticle::Create),
	m_trailSystem		(80.f, 20u, TrailParticle::Create),
	m_nodes				(nodes),
	m_audioManager		(config.audioManager)
{

	Ident = IdPlayer | playerId;
	
	//set up sprite from material
	sf::Vector2f halfTextureSize = sf::Vector2f(static_cast<float>(m_material.colourMap.getSize().x),
											static_cast<float>(m_material.colourMap.getSize().y));
	halfTextureSize /= 2.f;

	//shadow sprite
	m_shadowSprite.setTexture(m_material.shadowMap);
	m_shadowSprite.setOrigin(halfTextureSize);
	//colour sprite (rendered via shader)
	m_baseSprite.setTexture(m_material.colourMap);
	m_baseSprite.setOrigin(halfTextureSize);
	m_baseSprite.setPosition(m_material.textureSize / 2.f); //material texture size accounds for offset
	//render texture for shader rendering
	m_renderTexture.create(static_cast<unsigned>(m_material.textureSize.x), static_cast<unsigned>(m_material.textureSize.y));
	m_renderTexture.setSmooth(true);
	m_shadowOffset = ((m_material.textureSize / 2.f) - halfTextureSize).x; //offset is half size difference
	//main sprite based on render texture
	m_sprite = AniSprite(m_renderTexture.getTexture(), m_renderTexture.getSize().x, m_renderTexture.getSize().y);
	m_sprite.setOrigin(GetCentre());

	//load sounds
	m_engineSound.setBuffer(config.engineSound);
	m_engineSound.setLoop(true);
	m_engineSound.setMinDistance(1540.f);
	m_engineSound.setVolume(m_audioManager.GetGlobalEffectsVolume() * 0.35f);

	m_skidSound.setBuffer(m_audioManager.GetEffectBuffer("assets/sound/fx/vehicles/skid.wav"));
	m_skidSound.setMinDistance(340.f);

	m_impactSound.setBuffer(m_audioManager.GetEffectBuffer("assets/sound/fx/vehicles/collide.wav"));
	m_impactSound.setMinDistance(340.f);

	//collision points are relative to vehicle centre
	m_collisionPoints.push_back(-halfTextureSize);
	m_collisionPoints.push_back(sf::Vector2f(halfTextureSize.x, -halfTextureSize.y));
	m_collisionPoints.push_back(halfTextureSize);
	m_collisionPoints.push_back(sf::Vector2f(-halfTextureSize.x, halfTextureSize.y));

	//set up particle effects
	m_smokeSystem.SetStrength(130.f);
	m_smokeSystem.SetVelocity(sf::Vector2f(0.2f, -1.f));
	m_smokeSystem.SetTexture(m_material.smokeParticle);

	m_trailSystem.SetTexture(m_material.trailParticle);
	m_trailPoint.x = halfTextureSize.x / 2.f; //position for trail to start

	m_explosionSprite.setFrameRate(18.f);
	m_explosionSprite.setOrigin(128.f, 128.f); //MAGIC NUMBERS

	//calc lap length from nodes
	for(const auto& n : m_nodes)
		m_lapLength += n.NextMagnitude;

	//Reset();
}



void Vehicle::Update(float dt, std::vector<tmx::MapObject*>& objects)
{							
	//calc acceleration values based on current time
	m_acceleration = m_defAcceleration * dt;
	m_deceleration = m_defDeceleration * dt;
	m_forceReduction = m_defForceReduction * dt;

	//update particle effects
	m_smokeSystem.Update(dt);
	m_tyreSystem.Update(dt);
	m_trailSystem.Update(dt);
	m_explosionSprite.update();

	switch(m_state)
	{
	case Alive:
		//calc literal distance around lap
		m_lapDistance = 0.f;
		for(auto i = 0u; i < m_currentNode.ID; i++)
			m_lapDistance += m_nodes[i].NextMagnitude;

		//calc current distance around track
		m_nodeDistance = Helpers::Vectors::Dot(m_currentNode.NextUnit, (m_sprite.getPosition() - m_currentNode.Position));
		//TODO stop calc these when debug disabled
		m_debugPosition = m_currentNode.Position + (m_currentNode.NextUnit * m_nodeDistance);
		m_debugLengths.x = m_currentNode.NextMagnitude;
		m_debugLengths.y = m_nodeDistance;

		//update node if necessary - TODO this doesn't always move onto the next node when it should
		if(m_suggestNode && m_nodeDistance >= m_currentNode.NextMagnitude)
		{
			sf::Uint8 id = m_currentNode.ID;
			id++;
			if(id == m_nodes.size())
			{
				id = 0u;
				m_currentLap++;
				m_accumulatedDistance += (m_lapDistance + m_currentNode.NextMagnitude);
				//manually reset this when adding next node
				m_nodeDistance -= m_currentNode.NextMagnitude;
				m_lapDistance = 0.f;
				
				m_audioManager.PlayEffect(Audio::LapLine, m_nodes[0].Position);
			}

			m_currentNode = m_nodes[id];
			m_suggestNode = false;
		}
		//else if(m_suggestNode && m_nodeDistance < 0.f) -- attempt at foiling reverse driving
		//{
		//	sf::Uint8 id = m_currentNode.ID;
		//	if(id) id--;
		//	else
		//	{
		//		id = m_nodes.size() - 1;
		//		m_currentLap--;
		//		m_accumulatedDistance -= (m_lapDistance + m_currentNode.NextMagnitude); //this is untested
		//	}
		//	
		//	m_currentNode = m_nodes[id];
		//	m_suggestNode = false;
		//}

		m_currentDistance = m_accumulatedDistance + m_lapDistance + m_nodeDistance;
		
		//kill if neccessary
		if(m_health < 0.f) Kill();

		//restore health
		if(m_health < m_maxHealth) m_health += (m_healthIncrease * dt);
		else m_health = m_maxHealth;

		//check invincibility
		if(m_invincible &&
			m_invincibilityTimer.getElapsedTime().asSeconds() > m_invincibleTime)
			m_invincible = false;

		//parse the input first to set the conrol mask bits
		if(m_inputEnabled) m_ParseInput();

		//parse collision
		m_UpdateCollision(dt, objects);

		//turn first in case this modifies acceleration
		if(m_controlMask & LEFT) m_Rotate(-m_turnSpeed * dt);
		if(m_controlMask & RIGHT) m_Rotate(m_turnSpeed * dt);

		if(m_controlMask & ACCELERATE) m_Accelerate(dt);
		if(m_controlMask & DECELERATE) m_Brake(dt);

		//return turn speed in case of modification
		m_turnSpeed = m_defTurnSpeed;
	
		//do particle effects
		{
			if(m_type == Buggy) //rear wheels
			{
				sf::Uint8 randNum = Helpers::Random::Int(1, 2);
				m_smokeSystem.SetPosition(m_PointToWorld(m_collisionPoints[randNum]));
				m_tyreSystem.SetPosition(m_PointToWorld(m_collisionPoints[randNum]));
			}
			else //centre for buggy and ship
			{
				m_smokeSystem.SetPosition(m_sprite.getPosition());
				m_tyreSystem.SetPosition(m_sprite.getPosition());
			}
			m_tyreSystem.SetRotation(m_sprite.getRotation());
		}
		if((m_controlMask & HANDBRAKE)
			&& m_inputEnabled && m_currentSpeed != 0.f)
		{
			m_StartParticles();
			//play skid sound when not ship and handbraking
			if(m_type != Ship && m_skidSound.getStatus() != sf::Sound::Playing)
				m_skidSound.play(); 		
		}
		else 
		{
			m_StopParticles();
			m_skidSound.pause();//TODO fade edges to prevent clicks
		}

		break;
	case Falling:
		m_StopParticles();
		m_trailSystem.Stop();
		if(m_spriteScale >= 0.35f)
		{
			m_spriteScale -= 0.75f * dt;
			m_sprite.setScale(m_spriteScale, m_spriteScale);
			m_sprite.rotate(720.f * dt);
			if(m_colour) m_colour -= 280.f * dt;
			sf::Uint8 c = static_cast<sf::Uint8>(m_colour);
			m_sprite.setColor(sf::Color(c, c, c));
			m_explosionSprite.setColor(sf::Color(255u, 255u, 255u, static_cast<sf::Uint8>(255.f * m_spriteScale)));
		}
		else 
		{
			Kill();
		}
		break;
	case Dying: //play death anim and reset
		m_StopParticles();
		if(!m_explosionSprite.playing()) Reset();
		break;
	case Celebrating:
		m_StopParticles();
		m_trailSystem.Stop();
		m_velocity = sf::Vector2f();
		m_currentSpeed = 0.f;

		//cycle colour
		m_colour -= 280.f * dt;
		if(m_colour < 0.f) m_colour = 255.f - m_colour;

		sf::Uint8 r,g,b;
		r = static_cast<sf::Uint8>(m_colour);
		g = r - 85u;
		b = g - 85u;
		m_sprite.setColor(sf::Color(r, g, b));
		//do a little dance
		m_sprite.rotate(500.f * dt);
		
		break;
	case Eliminated: 
		//reset any systems if we died
		m_StopParticles();
		m_trailSystem.Stop();
		
		break;//return;
	default: break;
	}

	//update position 
	m_UpdatePosition(dt);		
	//update texture with new position
	m_UpdateTexture();
	//update trail
	m_trailSystem.SetPosition(m_PointToWorld(m_trailPoint));
	m_trailSystem.SetRotation(m_sprite.getRotation());

	//update engine sound pitch with speed
	m_engineSound.setPitch((m_currentSpeed / m_topSpeed) + 1.f);
	m_engineSound.setPosition(m_sprite.getPosition().x, -m_sprite.getPosition().y, 2.f);
	m_engineSound.setVolume(m_audioManager.GetGlobalEffectsVolume() * 0.35f);
	//std::cout << m_audioManager.GetGlobalEffectsVolume() << std::endl;
	m_skidSound.setPosition(m_sprite.getPosition().x, -m_sprite.getPosition().y, 0.5f);
	m_skidSound.setVolume(m_audioManager.GetGlobalEffectsVolume());
	m_impactSound.setVolume(m_audioManager.GetGlobalEffectsVolume());

	if((m_state != Alive && m_state != Falling)
		&& m_engineSound.getStatus() == sf::Sound::Playing) m_engineSound.stop();
}

void Vehicle::Impact(float dt, Vehicle& vehicle)
{
	//skip dead / eliminated vehicles
	if(m_state != Alive || vehicle.GetState() != Alive) return;

	//get vehicle radius to check for potential collision
	float radius = Helpers::Vectors::GetLength(m_collisionPoints[0]);
	float distance = Helpers::Vectors::GetLength(m_sprite.getPosition() - vehicle.GetCentre()) / 2.f;

	if(radius > distance) //cars are close enough to collide
	{
		//check if our collision points interset other vehicle
		for(auto point = m_collisionPoints.begin(); point != m_collisionPoints.end(); ++point)
		{
			if(vehicle.m_Contains(m_PointToWorld(*point)))
			{
				//we have a collision 
				m_SeparateVehicle(dt, vehicle);
				return; //skip the rest of the test
			}
		}

		//if none of our points intersected, test the other car's
		for(auto point = vehicle.m_collisionPoints.begin(); point != vehicle.m_collisionPoints.end(); ++point)
		{
			if(m_Contains(vehicle.m_PointToWorld(*point)))
			{
				//collision
				m_SeparateVehicle(dt, vehicle);
				return;
			}
		}
	}
}

void Vehicle::Kill(State state)
{
	m_explosionSprite.play(0, m_explosionSprite.getFrameCount() - 1, false);
	m_explosionSprite.setPosition(m_sprite.getPosition());
	m_explosionSprite.setScale(m_spriteScale, m_spriteScale); //so smaller when fallen off
	if(state != Eliminated)
		m_audioManager.PlayEffect(Audio::Explode, m_sprite.getPosition());

	m_sprite.setColor(sf::Color::White);
	m_state = state;
	m_inputEnabled = false;

	m_trailSystem.Stop();
}

void Vehicle::Reset()
{
	//offset by spawn position so vehicles don't spawn on top of each other
	float offset, padding = m_material.textureSize.y + 16.f;
	sf::Vector2f spawnPos = m_currentNode.Position;
	spawnPos.y += 10.f;
	
	offset = (padding * m_spawnPosition) - (padding / 2.f);
	spawnPos.x += offset;
	if(m_spawnPosition > 1)
	{
		//move to back row
		spawnPos.x -= padding * 2.f;
		spawnPos.y += m_material.textureSize.x + 40.f;
	}

	sf::Transform tf;
	tf.rotate(m_currentNode.SpawnAngle - 90.f, m_currentNode.Position);
	spawnPos = tf.transformPoint(spawnPos);

	m_currentSpeed = 0.f;
	m_forceStrength = 0.f;
	m_velocity = sf::Vector2f();
	m_externalForce = sf::Vector2f();
	m_sprite.setPosition(spawnPos);
	m_spriteScale = 1.f;
	m_sprite.setScale(m_spriteScale, m_spriteScale);
	m_sprite.setRotation(m_currentNode.SpawnAngle);
	m_sprite.setColor(sf::Color::White);
	m_explosionSprite.setColor(sf::Color::White);
	m_colour = 255.f;
	m_controlMask = 0u; //important! :D

	m_state = Alive;
	m_health = m_maxHealth;
	m_engineSound.play();

	//reset AI clock to delay its start
	Ai.Delay();

	//start trail
	m_trailSystem.Start();
}