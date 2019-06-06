///source definitions for vehicle manager class///
#include <Game/VehicleManager.h>
#include <Game/Shaders/Normal.h>
#include <Game/Shaders/BlendOverlay.h>
#include <sstream>

using namespace Game;

//ctor
VehicleManager::VehicleManager(SharedData& sharedData)
	: m_textureResource		(sharedData.Textures),
	m_audioManager			(sharedData.AudioManager),
	m_gameData				(sharedData.Game),
	m_mapData				(sharedData.Map),
	m_screenData			(sharedData.Screen),
	m_roundEndDuration		(3.f), //number of seconds to celebrate
	m_playerBounds			(0.f, 0.f, 1080.f, 1080.f),
	m_starSystem			(50.f, 80u, StarParticle::Create),
	m_raceOver				(false),
	m_altRules				(false),
	m_prevRaceStarted		(false),
	m_resetTime				(2.5f), //if all players are eliminated wait this long before round ends
	m_debugText				("", sharedData.Fonts.Get("assets/fonts/default.ttf"), 40u),
	m_carConfig				(m_audioManager),
	m_shipConfig			(m_audioManager),
	m_bikeConfig			(m_audioManager)
{
	m_trackNodes.reserve(50);
	m_starSystem.SetStrength(440.f);

	m_debugText.setColor(sf::Color(240u, 120u, 0u));
	m_nodeLines.setPrimitiveType(sf::LinesStrip);
	m_vehicleLines.setPrimitiveType(sf::Lines);
}

//dtor


//public functions
void VehicleManager::ParseMapNodes(tmx::MapLayer& nodeLayer, bool reverse)
{
	//Console::lout << "Vehicle manager parsing map nodes..." << std::endl;
	
	//clear existing nodes
	m_trackNodes.clear();	
	m_nodeLines.clear();

	
	//skip if we tried to parse wrong layer
	if(nodeLayer.name != "Waypoints") return; 


	if(reverse) //reverse node name order
	{
		std::vector< std::string> IDs;
		for(const auto& i : nodeLayer.objects)
			IDs.push_back(i.GetName());

		std::reverse(IDs.begin() + 1, IDs.end());

		for(auto i = 0u; i < nodeLayer.objects.size(); ++i)
			nodeLayer.objects[i].SetName(IDs[i]);
	}


	//create nodes from layer
	for(auto object = nodeLayer.objects.begin(); object != nodeLayer.objects.end(); ++object)
	{
		std::string name = object->GetName();
		if(!name.empty())
		{
			try
			{
				TrackNode node;
				node.ID = std::stoi(name);
				node.Position = object->GetCentre();
				m_trackNodes.push_back(node);
				//std::cout << name << std::endl;
			}
			catch(std::invalid_argument& ex)
			{
				std::cerr << "Node " << name << " contains invalid name." << ex.what() << std::endl;
			}
		}
	}

	//sort nodes correctly
	std::sort(m_trackNodes.begin(), m_trackNodes.end(), 
		[](const TrackNode& a, const TrackNode& b)->bool{return (a.ID < b.ID);});


	float lapLength = 0.f; //store the total length of one lap
	//calc spawn rotation and normalized vect to next node for distance calc
	for(auto node = m_trackNodes.begin(); node != m_trackNodes.end(); ++node)
	{
		if(node == m_trackNodes.end() -1)
			node->NextUnit = m_trackNodes.begin()->Position;// - node->Position;
		else
			node->NextUnit = (node + 1)->Position;// - node->Position;

		//align node ID with index incase of mis-numbering
		node->ID = std::distance(m_trackNodes.begin(), node); 
		//set rotation angle based in vector to next node
		node->SpawnAngle = Helpers::Vectors::GetAngle(node->Position - node->NextUnit);
		//normalise next position save doing it every update
		node->NextUnit -= node->Position;
		node->NextMagnitude = Helpers::Vectors::GetLength(node->NextUnit);
		//Console::lout << "Node mag: " << node->NextMagnitude << std::endl;
		node->NextUnit = Helpers::Vectors::Normalize(node->NextUnit);

		//append to current lap length
		lapLength += node->NextMagnitude;

		//add nodes for debug output
		m_nodeLines.append(sf::Vertex(node->Position, sf::Color::Green));
	}
	m_nodeLines.append(m_nodeLines[0]); //close the loop
	m_mapData.LapLength = lapLength;
}

bool VehicleManager::CreateVehicles()
{
	//Console::lout << "Creating player vehicles" << std::endl;
	
	//clear existing vehicles
	m_vehicles.clear();	
	
	if(m_trackNodes.empty())
	{
		std::cerr << "No track nodes found, no vehicles created." << std:: endl;
		return false;
	}
	else if(m_gameData.Players.empty())
	{
		std::cerr << "Invalid or no player data found, no vehicles created." << std::endl;
		return false;
	}
	
	//update info from loaded map
	m_LoadMaps();
	m_carConfig.material.lightPosition = m_mapData.LightPosition; //if loading maps here can we move to load maps?
	m_shipConfig.material.lightPosition = m_mapData.LightPosition;
	m_bikeConfig.material.lightPosition = m_mapData.LightPosition;

	//create cars
	sf::Uint8 id = 0u;
	for(const auto& i : m_gameData.Players)
	{
		std::unique_ptr<Vehicle> v;
		switch (i->Vehicle)
		{
		default:
		case Vehicle::Buggy:
			v = std::unique_ptr<Vehicle>(new Vehicle(m_carConfig, m_trackNodes, id, i->Controls, i->CpuIntelligence));
			break;
		case Vehicle::Ship:
			v = std::unique_ptr<Vehicle>(new Vehicle(m_shipConfig, m_trackNodes, id, i->Controls, i->CpuIntelligence));
			break;
		case Vehicle::Bike:
			v = std::unique_ptr<Vehicle>(new Vehicle(m_bikeConfig, m_trackNodes, id, i->Controls, i->CpuIntelligence));
			break;
		}
	
		v->SetSpawnPosition(i->GridPosition);
		v->Reset(); // applies spawn position - TODO skip in network games
		m_vehicles.push_back(std::move(v));
		id++;
	}

	//set views / rules based on game mode
	switch(m_gameData.Rules)
	{
	case GameRules::Elimination:
		//check if 2 player - TODO: this better
		m_altRules = (id < 3u);
		//assign update function for current set of rules
		m_UpdateGameRules = [this](float dt, tmx::MapLoader& map) 
		{
			m_UpdateLocalMulti(dt, map);
		};
		m_audioManager.SetAttenuation(4.f);

		m_screenData.RaceViews[0].setCenter(m_trackNodes[0].Position);
		m_SetPlayerBounds(m_screenData.RaceViews[0].getCenter());

		break;
	case GameRules::TimeTrial:
		m_UpdateGameRules = [this](float dt, tmx::MapLoader& map)
		{
			m_UpdateTimeTrial(dt, map);
		};
		m_audioManager.SetAttenuation(4.f);

		m_screenData.RaceViews[0].setCenter(m_trackNodes[0].Position);
		break;
	case GameRules::SplitScreen:
		assert(m_vehicles.size() <= m_screenData.RaceViews.size());
		m_UpdateGameRules = [this](float dt, tmx::MapLoader& map)
		{
			m_UpdateSplitScreen(dt, map);
		};
		m_audioManager.SetListenerPosition(m_mapData.LightPosition);
		m_audioManager.SetAttenuation(0.f);

		for(auto& v : m_screenData.RaceViews)
			v.setCenter(m_mapData.LightPosition);
		break;
	}


	//make sure star particles are properly dead from any previous races
	m_starSystem.Stop();
	m_starSystem.Kill();

	m_raceOver = false;
	return true; //so we can test vehicles are loaded
}

void VehicleManager::EndRace()
{
	if(m_gameData.CurrentRoundState != RaceWon) return;
	m_raceOver = true;
	m_starSystem.Stop();
}

void VehicleManager::Update(float dt, tmx::MapLoader& map)
{
	if(m_vehicles.empty()) return;
	
	//m_UpdateLocalMulti(dt, map);
	m_UpdateGameRules(dt, map);

	//update particle system
	m_starSystem.Update(dt);
	m_starSystem.SetVelocity(sf::Vector2f(Helpers::Random::Float(-1.f, 1.f), Helpers::Random::Float(-1.f, 1.f)));

	//update materials
	m_carConfig.material.cameraPosition = m_screenData.RaceViews[0].getCenter() * m_screenData.WindowScale;
	m_shipConfig.material.cameraPosition = m_bikeConfig.material.cameraPosition = m_carConfig.material.cameraPosition;
}

void VehicleManager::Draw(sf::RenderTarget& rt, bool falling)
{
	//draw particles first
	rt.draw(m_starSystem, sf::BlendAdd);
	//particles need to appear under ALL vehicles
	for(auto&& vehicle : m_vehicles)
		vehicle->DrawParticles(rt);

	//then draw cars
	for(auto&& vehicle : m_vehicles)
	{
		if((vehicle->GetState() == Falling) == falling)
			rt.draw(*vehicle);
	}
}

void VehicleManager::DrawDebug(sf::RenderTarget& rt)
{
	m_vehicleLines.clear();
	for(auto&& v : m_vehicles)
	{
		std::stringstream s;
		s.precision(4);
		s << (int)v->GetCurrentDistance() << std::endl << std::endl;
		//s << v->GetDebugLengths().x << ", " << v->GetDebugLengths().y;
		m_debugText.setString(s.str());
		m_debugText.setPosition(v->GetCentre() - sf::Vector2f(90.f, 90.f));
		rt.draw(m_debugText);

		sf::Color c = PlayerColours::Light(v->Ident & UID);
		m_vehicleLines.append(sf::Vertex(v->GetCentre(), c));
		m_vehicleLines.append(sf::Vertex(v->GetDebugPos(), c));
	}

	rt.draw(m_nodeLines);
	rt.draw(m_vehicleLines);
}

//private functions
void VehicleManager::m_SetPlayerBounds(const sf::Vector2f& centre)
{
	float offset = m_screenData.ViewSize. y / 2.f;
	m_playerBounds.top = centre.y - offset;
	m_playerBounds.left = centre.x - offset;
}

void VehicleManager::m_LoadMaps()
{
	//Console::lout <<"Creating vehicle materials... " << std::endl;
	sf::Uint8 shadowOffset = 20u;


	//-------create configs for each vehicle type------------//
			//-----------------car------------------//
	m_carConfig.material.colourMap = m_textureResource.Get("assets/Textures/Vehicles/buggy/buggy_diffuse.png");
	m_carConfig.material.normalMap = m_textureResource.Get("assets/Textures/Vehicles/buggy/buggy_normal.tga");
	m_carConfig.material.specularMap = m_textureResource.Get("assets/Textures/Vehicles/buggy/buggy_specular.png");
	m_carConfig.material.shadowMap = m_textureResource.Get("assets/Textures/Vehicles/buggy/buggy_shadow.png");
	m_carConfig.material.reflectMap = m_textureResource.Get("assets/Textures/Environment/clouds.png");
	m_carConfig.material.neonMap = m_textureResource.Get("assets/Textures/Vehicles/buggy/buggy_neon.png");
	m_carConfig.material.explosion = m_textureResource.Get("assets/Textures/Vehicles/explode01.png");
	m_carConfig.material.trailParticle = m_textureResource.Get("assets/Textures/Vehicles/trail_particle.png");
	m_carConfig.material.smokeParticle = m_textureResource.Get("assets/Textures/Vehicles/skid_puff.png");


	m_carConfig.material.textureSize = sf::Vector2f(static_cast<float>(m_carConfig.material.colourMap.getSize().x + shadowOffset),
		static_cast<float>(m_carConfig.material.colourMap.getSize().y + shadowOffset)); //includes offset for render texture
	m_carConfig.material.shadowOffset = static_cast<float>(shadowOffset) / 2.f;
			//---------set up shaders---------------//
	m_carConfig.material.normalShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_carConfig.material.normalShader->loadFromMemory(NormalMap, sf::Shader::Fragment);
	m_carConfig.material.normalShader->setParameter("colourMap", m_carConfig.material.colourMap);
	m_carConfig.material.normalShader->setParameter("normalMap", m_carConfig.material.normalMap);
	m_carConfig.material.normalShader->setParameter("reflectMap", m_carConfig.material.reflectMap);
	m_carConfig.material.normalShader->setParameter("specularMap", m_carConfig.material.specularMap);
	m_carConfig.material.normalShader->setParameter("neonMap", m_carConfig.material.neonMap);
	m_carConfig.material.normalShader->setParameter("resolution", m_carConfig.material.textureSize); //shader is applied relative to render texture, not screen
	
	m_carConfig.material.trailShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_carConfig.material.trailShader->loadFromMemory(blendOverlay, sf::Shader::Fragment);
	m_carConfig.material.trailShader->setParameter("baseMap", m_carConfig.material.trailParticle);

			//----------set up handling------------//
	m_carConfig.handbrakeAmount = 22.f;
	m_carConfig.topSpeed = 980.f;
	m_carConfig.turnSpeed = 70.f;
	m_carConfig.maxHealth = 29.f;
		//------handling set to buggy by default------//
	m_carConfig.engineSound = m_audioManager.GetEffectBuffer("assets/sound/fx/vehicles/e03.wav");
	

			//----------------ship------------------//
	shadowOffset = 40u;
	m_shipConfig.material.colourMap = m_textureResource.Get("assets/Textures/Vehicles/ship/ship_diffuse.png");
	m_shipConfig.material.normalMap = m_textureResource.Get("assets/Textures/Vehicles/ship/ship_normal.tga");
	m_shipConfig.material.specularMap = m_textureResource.Get("assets/Textures/Vehicles/ship/ship_specular.png");
	m_shipConfig.material.shadowMap = m_textureResource.Get("assets/Textures/Vehicles/ship/ship_shadow.png");
	m_shipConfig.material.reflectMap = m_textureResource.Get("assets/Textures/Environment/clouds.png");
	m_shipConfig.material.neonMap = m_textureResource.Get("assets/Textures/Vehicles/ship/ship_neon.png");
	m_shipConfig.material.explosion = m_textureResource.Get("assets/Textures/Vehicles/explode01.png");
	m_shipConfig.material.trailParticle = m_textureResource.Get("assets/Textures/Vehicles/trail_particle.png");
	m_shipConfig.material.smokeParticle = m_textureResource.Get("assets/Textures/Vehicles/skid_puff.png");

	m_shipConfig.material.textureSize = sf::Vector2f(static_cast<float>(m_shipConfig.material.colourMap.getSize().x + shadowOffset),
		static_cast<float>(m_shipConfig.material.colourMap.getSize().y + shadowOffset)); //includes offset for render texture
	m_shipConfig.material.shadowOffset = static_cast<float>(shadowOffset) / 2.f;
			//---------set up shader---------------//
	m_shipConfig.material.normalShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_shipConfig.material.normalShader->loadFromMemory(NormalMap, sf::Shader::Fragment);
	m_shipConfig.material.normalShader->setParameter("normalMap", m_shipConfig.material.normalMap);
	m_shipConfig.material.normalShader->setParameter("reflectMap", m_shipConfig.material.reflectMap);
	m_shipConfig.material.normalShader->setParameter("specularMap", m_shipConfig.material.specularMap);
	m_shipConfig.material.normalShader->setParameter("neonMap", m_shipConfig.material.neonMap);
	m_shipConfig.material.normalShader->setParameter("resolution", m_shipConfig.material.textureSize/* * m_screenData.WindowScale*/); //shader is applied relative to render texture, not screen
	
	m_shipConfig.material.trailShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_shipConfig.material.trailShader->loadFromMemory(blendOverlay, sf::Shader::Fragment);
	m_shipConfig.material.trailShader->setParameter("baseMap", m_shipConfig.material.trailParticle);
	
		//----------set up handling------------//
	m_shipConfig.type = Vehicle::Ship;
	m_shipConfig.defDriftAmount =  0.8f;
	m_shipConfig.driftAmount = m_shipConfig.defDriftAmount;
	m_shipConfig.handbrakeAmount = 14.f;
	m_shipConfig.turnSpeed = 66.f;
	m_shipConfig.deceleration = 550.f;
	m_shipConfig.acceleration = 2200.f;
	m_shipConfig.mass = 90.f;
	m_shipConfig.topSpeed  = 1050.f;
	m_shipConfig.maxHealth = 160.f;

	m_shipConfig.engineSound = m_audioManager.GetEffectBuffer("assets/sound/fx/vehicles/e02.wav");

			//----------------bike------------------//
	shadowOffset = 30u;
	m_bikeConfig.material.colourMap = m_textureResource.Get("assets/Textures/Vehicles/bike/bike_diffuse.png");
	m_bikeConfig.material.normalMap = m_textureResource.Get("assets/Textures/Vehicles/bike/bike_normal.tga");
	m_bikeConfig.material.specularMap = m_textureResource.Get("assets/Textures/Vehicles/bike/bike_specular.png");
	m_bikeConfig.material.shadowMap = m_textureResource.Get("assets/Textures/Vehicles/bike/bike_shadow.png");
	m_bikeConfig.material.reflectMap = m_textureResource.Get("assets/Textures/Environment/clouds.png");
	m_bikeConfig.material.neonMap = m_textureResource.Get("assets/Textures/Vehicles/bike/bike_neon.png");
	m_bikeConfig.material.explosion = m_textureResource.Get("assets/Textures/Vehicles/explode01.png");
	m_bikeConfig.material.trailParticle = m_textureResource.Get("assets/Textures/Vehicles/trail_particle.png");
	m_bikeConfig.material.smokeParticle = m_textureResource.Get("assets/Textures/Vehicles/skid_puff.png");

	m_bikeConfig.material.textureSize = sf::Vector2f(static_cast<float>(m_bikeConfig.material.colourMap.getSize().x + shadowOffset),
		static_cast<float>(m_bikeConfig.material.colourMap.getSize().y + shadowOffset)); //includes offset for render texture
	m_bikeConfig.material.shadowOffset = static_cast<float>(shadowOffset) / 2.f;
			//---------set up shader---------------//
	m_bikeConfig.material.normalShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_bikeConfig.material.normalShader->loadFromMemory(NormalMap, sf::Shader::Fragment);
	m_bikeConfig.material.normalShader->setParameter("normalMap", m_bikeConfig.material.normalMap);
	m_bikeConfig.material.normalShader->setParameter("reflectMap", m_bikeConfig.material.reflectMap);
	m_bikeConfig.material.normalShader->setParameter("specularMap", m_bikeConfig.material.specularMap);
	m_bikeConfig.material.normalShader->setParameter("neonMap", m_bikeConfig.material.neonMap);
	m_bikeConfig.material.normalShader->setParameter("resolution", m_bikeConfig.material.textureSize/* * m_screenData.WindowScale*/); //shader is applied relative to render texture, not screen
	
	m_bikeConfig.material.trailShader = std::unique_ptr<sf::Shader>(new sf::Shader());
	m_bikeConfig.material.trailShader->loadFromMemory(blendOverlay, sf::Shader::Fragment);
	m_bikeConfig.material.trailShader->setParameter("baseMap", m_bikeConfig.material.trailParticle);
	
		//----------set up handling------------//
	m_bikeConfig.type = Vehicle::Bike;
	m_bikeConfig.defDriftAmount = 0.09f;
	m_bikeConfig.driftAmount = m_bikeConfig.defDriftAmount;
	m_bikeConfig.handbrakeAmount = 20.f;
	m_bikeConfig.turnSpeed = 100.f;
	m_bikeConfig.deceleration = 2000.f;
	m_bikeConfig.acceleration = 5500.f;
	m_bikeConfig.maxHealth = 8.f; //bikes handle well but damage easily
	m_bikeConfig.mass = 30.f;
	m_bikeConfig.topSpeed = 984.f;

	m_bikeConfig.engineSound = m_audioManager.GetEffectBuffer("assets/sound/fx/vehicles/e04.wav");

	//Console::lout << "Finished." << std::endl;
}

void VehicleManager::m_ResetAll(const Vehicle* leader)
{
	//get node to reset to
	TrackNode node = leader->GetCurrentNode();
	sf::Int8 lap = leader->GetCurrentLap();
	//set node as camera destination
	m_resetTarget = node.Position;				
				
	//reset vehicles
	for(auto&& vehicle : m_vehicles)
	{
		vehicle->SetCurrentNode(node);
		vehicle->SetCurrentLap(lap); //update any vehicles which may have been lapped while eliminated
		vehicle->EnableInput(false); //prevent moving until round started again
		vehicle->Reset();
	}				
	m_gameData.CurrentRoundState = RoundReset;
}

bool VehicleManager::m_UpdateCamera(float dt, const sf::Vector2f& dest, sf::Uint8 viewId)
{
	sf::Vector2f target = dest - m_screenData.RaceViews[viewId].getCenter();
	const float distance = Helpers::Vectors::GetLength(target);
	const float maxDist = m_screenData.RaceViews[viewId].getSize().y / 4.f;
	const float step = 1400.f / maxDist;
	float speed = step * distance;
	sf::Vector2f movement = Helpers::Vectors::Normalize(target)* speed * dt;
	m_screenData.RaceViews[viewId].move(movement);

	return (distance < 10.f); //returns true if camera reaches target
}

void VehicleManager::m_StartCelebrating(sf::Uint8 vehicleId)
{
	m_roundTimer.restart();
	m_gameData.CurrentRoundState = RoundEnd;

	if(m_vehicles[vehicleId]->GetState() != Alive)
		m_vehicles[vehicleId]->GetSprite().setPosition(m_screenData.RaceViews[0].getCenter());

	m_vehicles[vehicleId]->SetState(Celebrating);
	m_starSystem.SetPosition(m_vehicles[vehicleId]->GetCentre());
	m_cameraDestination = m_vehicles[vehicleId]->GetCentre();
	m_starSystem.Start();

	//store ID of player who won point for HUD
	m_gameData.WinnerId = vehicleId;

	m_audioManager.PlayEffect(Audio::Success, m_vehicles[vehicleId]->GetCentre());
}