#ifndef VEHICLE_H_
#define VEHICLE_H_

#include <Game/AudioManager.h>
#include <Game/PhysEntity.h>
#include <Game/TrackNode.h>
#include <Game/SmokeParticle.h>
#include <Game/SkidParticle.h>
#include <Game/TrailParticle.h>

#include <tmx/MapObject.h>

#include<SFML/Graphics/Shader.hpp>

#include <Helpers.h>

namespace Game
{
	//represents a set of textures and a shader for rendering vehicle
	struct Material
	{
		Material() : shadowOffset (10.f){};
		sf::Texture colourMap, neonMap, explosion, trailParticle;
		sf::Texture shadowMap, normalMap, specularMap, reflectMap;
		sf::Texture smokeParticle;
		std::unique_ptr<sf::Shader> normalShader, trailShader;
		sf::Vector2f lightPosition; //could make this const with a ctor
		sf::Vector2f textureSize; //width height of texture
		sf::Vector2f cameraPosition;
		float shadowOffset;
	};

	class Vehicle : public PhysEntity, public sf::Drawable
	{	
	public:
		enum VehicleType
		{
			Ship = 0u,
			Buggy = 1u,
			Bike = 2u
		};

		struct Config //used to set up vehicle type, including material
		{
			Config(AudioManager& a) : //default values are for car
				audioManager	(a),
				topSpeed		(1000.f),
				turnSpeed		(80.f),
				acceleration	(4000.f),
				deceleration	(1500.f),
				handbrakeAmount	(18.f),
				maxHealth		(100.f),
				defDriftAmount	(0.7f),
				driftAmount		(0.5f),
				mass			(46.f),
				type			(Buggy)
			{};
			float topSpeed; //units per second, default: 1100
			float turnSpeed; //degrees per second default: 80
			float acceleration; //units per second^2 default: 4000
			float deceleration; //units per second^2 default: 1500
			float handbrakeAmount; //reduction in speed per second, def: 18
			float maxHealth;
			float defDriftAmount; //default amount when not braking
			float driftAmount; //0 no drift, 1 full drift
			float mass;
			Material material;
			VehicleType type;
			sf::SoundBuffer engineSound; //different vehicles have different engine sounds
			AudioManager& audioManager;
		};

		Vehicle(Config& config, const std::vector<TrackNode>& nodes, sf::Uint8 playerId = 0u, ControlSet controls = KeySetOne, Intelligence intelligence = Human);

		void Update(float dt, std::vector<tmx::MapObject*>& objects);
		const TrackNode GetCurrentNode() const{return m_currentNode;};
		void SetCurrentNode(const TrackNode& node){m_currentNode = node;};
		void SetRacePosition(sf::Uint8 position){ m_racePosition = position;};
		const sf::Uint8 GetRacePosition() const{return m_racePosition;};
		void SetCurrentLap(sf::Int8 lap){m_currentLap = lap; m_accumulatedDistance = m_lapLength * lap;};
		const sf::Int8 GetCurrentLap() const{return m_currentLap;};
		const float GetCurrentDistance() const{return m_currentDistance;};
		const float GetLapDistance() const{return m_lapDistance + m_nodeDistance;};
		void EnableInput(bool enabled)
		{
			if(enabled && !m_inputEnabled)
			{
				//become briefly invincible after re-enabling controls
				m_invincible = true;
				m_invincibilityTimer.restart();
			}
			m_inputEnabled = enabled;
		};
		bool Invincible() const {return m_invincible || !m_inputEnabled;};
		//applies an impact force to another vehicle if it collides with this one
		void Impact(float dt, Vehicle& vehicle);
		//overload for 'roids / debris
		void Impact(const sf::Vector2f& velocity, float damage = 0.f)
		{
			m_AddForce(velocity, Helpers::Vectors::GetLength(velocity) * 0.55f);
			if(m_impactClock.getElapsedTime().asSeconds() > m_impactLimit)
			{
				m_health -= damage;			
				m_PlayImpactSound();
				m_impactClock.restart();
			}
		}
		//Kills the vehicle
		void Kill(State state = Dying);
		//does a hard reset
		void Reset();
		//draws particle effects
		void DrawParticles(sf::RenderTarget& rt)
		{
			rt.draw(m_tyreSystem);
			rt.draw(m_smokeSystem, sf::BlendAdd);
			m_material.trailShader->setParameter("colour", PlayerColours::Light(m_playerId));
			sf::RenderStates r(sf::BlendAdd);
			r.shader = m_material.trailShader.get();
			rt.draw(m_trailSystem, r);
		};
		float GetRadius() const{return m_sprite.getLocalBounds().height / 2.f;}; //TODO calc this more nicerer
		const sf::Vector2f GetVelocity() const{return m_velocity * m_currentSpeed;};
		float GetMass() const{return m_mass;};
		const sf::Vector2f GetDebugPos() const{return m_debugPosition;};
		const sf::Vector2f GetDebugLengths() const{return m_debugLengths;};
		//sets which order vehicles spawn in
		void SetSpawnPosition(sf::Uint8 position){m_spawnPosition = static_cast<float>(position);};
		//sets vehicle physical position
		void SetPosition(const sf::Vector2f& position){m_sprite.setPosition(position);};
	private:
		//reference to list of current map track nodes
		const std::vector<TrackNode>& m_nodes;
		//type of vehicle
		VehicleType m_type;
		//allow control from various sources
		ControlSet m_controlSet;
		sf::Uint8 m_controlMask;
		//consts for keymasking
		const sf::Uint8 ACCELERATE;
		const sf::Uint8 DECELERATE;
		const sf::Uint8 LEFT;
		const sf::Uint8 RIGHT;
		const sf::Uint8 HANDBRAKE;
		const sf::Uint8 TURBO;

		sf::Vector2f m_frontNormal, m_sideNormal;
		const sf::Vector2f FORWARD_VECTOR;
		const sf::Vector2f SIDE_VECTOR;
		const float m_defTopSpeed;
		const float m_defTurnSpeed;
		const float m_defAcceleration; //actually might better serve as config var (ie not const)
		const float m_defDeceleration;
		const float m_defForceReduction;
		const float m_handbrakeAmount; //reduction in speed per second
		const float m_maxHealth;
		const float m_healthIncrease; //amount to increase health per second
		const float m_mass; //used in collision calcs

		float m_acceleration;
		float m_deceleration;
		float m_currentSpeed; //velocity multiplier
		float m_topSpeed; //maximum speed
		float m_turnSpeed; //degrees to turn per second
		float m_zHeight; //distance above ground. positive values mean wheels in air
		float m_jumpPower; //amount to modify z height
		float m_spriteScale;
		float m_health;
		float m_defDriftAmount; //default amount when not modified
		float m_driftAmount; //0 no drift, 1 full drift
		float m_lapDistance; //how far vehicle is along current lap
		float m_nodeDistance; //how far from current node
		float m_accumulatedDistance; 
		float m_spawnPosition; //affects order in which vehicles spawn
		float m_lapLength; //total length of current map length

		sf::Uint8 m_racePosition;
		TrackNode m_currentNode;
		sf::Int8 m_currentLap;
		float m_currentDistance;
		sf::Uint8 m_playerId;
		float m_colour; //for darkening when falling
		bool m_inputEnabled; //force waiting at start
		bool m_invincible; //temp invincibility
		sf::Clock m_invincibilityTimer;
		const float m_invincibleTime;
		bool m_suggestNode; //collision has suggested we switch nodes

		//these sounds are treated as a special case but still use the resource manager of the audio manager
		sf::Sound m_engineSound;
		sf::Sound m_skidSound, m_impactSound;
		AudioManager& m_audioManager;
		void m_PlayImpactSound() //makes sure sound only plays once
		{
			if(m_impactSound.getStatus() != sf::Sound::Playing)
			{
				m_impactSound.setPosition(m_sprite.getPosition().x, -m_sprite.getPosition().y, 2.f);
				m_impactSound.play();
			}
		}

		sf::Clock m_impactClock; //provides a time limit to prevent multiple hits
		const float m_impactLimit;

		bool m_inCollision; //prevent multiple collision handling of same collision with environment

		//stuff for visuals
		sf::RenderTexture m_renderTexture;
		Material& m_material;
		sf::Sprite m_baseSprite, m_shadowSprite;
		//animated explosion sprite
		AniSprite m_explosionSprite;
		float m_shadowOffset; //using the base sprite we can give the appearance of jumping without altering main sprite position
		const float m_lightZ; //z value for shader light position
		//smoke particles and tyre skids
		ParticleSystem m_smokeSystem;
		ParticleSystem m_tyreSystem;
		ParticleSystem m_trailSystem;
		sf::Vector2f m_trailPoint;

		void m_Accelerate(float dt);
		void m_Brake(float dt);
		void m_Rotate(float angle);

		//returns normal of given direction rotated about vehicle
		const sf::Vector2f m_GetNormal(const sf::Vector2f& direction) const;

		//updates the velocity vector when accelerating or braking
		void m_UpdateVelocity();
		//parses input from controller/keyboard/network
		void m_ParseInput();
		//parses controller data by given controller number
		void m_ParseController(sf::Uint8 id);

		//parses any reaction to supplied map objects
		void m_UpdateCollision(float dt, std::vector<tmx::MapObject*>& objects);
		//updates the vehicle position based on input
		void m_UpdatePosition(float dt);
		//update the sprite effects with normal map rendering
		void m_UpdateTexture();
		//causes vehicle to jump
		void m_Jump(float jumpPower);
		//separates vehicles during collision
		void m_SeparateVehicle(float dt, Vehicle& vehicle);
		//utilities for start / stopping cars particle systems
		void m_StartParticles();
		void m_StopParticles();

		//debug out put stuff
		sf::Vector2f m_debugPosition;
		sf::Vector2f m_debugLengths;

		//override for drawable
		void draw(sf::RenderTarget& rt, sf::RenderStates states)const;

		class Ai : private sf::NonCopyable
		{
		public:
			Ai(Vehicle& parent, Intelligence intelligence)
				: m_parent(parent),
				m_topSpeedMultiplier(1.f),
				m_brakingDistance(195.f),
				m_delayTime(0.4f),
				m_alterTimeout(15.f)
			{
				switch(intelligence)
				{
				case Smart:
					m_brakingDistance = 170.f;
					m_delayTime = 0.5f;
					m_alterTimeout = 25.f;
					break;
				case Average:
					m_topSpeedMultiplier = 0.996f;
					m_brakingDistance = 185.f;
					m_delayTime = 0.5f;
					m_alterTimeout = 16.f;
					break;
				case Dumb:
					m_topSpeedMultiplier = 0.99f;
					m_brakingDistance = 200.f;
					m_delayTime = 0.4f;
					m_alterTimeout = 6.f;
					break;
				default: break;
				}
			};

			void Update(); //should only need vehicle current node
			void Delay(){m_delayClock.restart();};
		private:
			Vehicle& m_parent;

			float m_topSpeedMultiplier; //overall top speed of AI
			float m_brakingDistance; //distance from node before AI starts braking
			float m_delayTime; //delay in seconds after controls enabled

			sf::Clock m_delayClock, m_alterClock;
			float m_alterTimeout;

		} Ai;

		friend class Ai;
	};

};


#endif //VEHICLE_H_