///class for managing vehicles on a map///
#ifndef VEHICLE_MANAGER_H_
#define VEHICLE_MANAGER_H_

#include <Game/Vehicle.h>
#include <Game/StarParticle.h>
#include <Game/SharedData.h>
#include <Game/EnergyFence.h>

#include <tmx/MapLoader.h>

#include <functional>

namespace Game
{
	class VehicleManager : private sf::NonCopyable
	{
	public:
		VehicleManager(SharedData& sharedData);

		void ParseMapNodes(tmx::MapLayer& nodeLayer, bool reverse = false);
		bool CreateVehicles();
		void EndRace();
		void Update(float dt, tmx::MapLoader& map);
		void Draw(sf::RenderTarget& rt, bool falling); //so we can split drawing falling cars from alive cars. A bit hacky
		bool RaceOver()const{return m_raceOver;};
		void Reset(){m_vehicles.clear();};

		std::vector< std::unique_ptr<Vehicle> >& GetVehicles(){return m_vehicles;};

		void DrawDebug(sf::RenderTarget& rt);

	private:

		TextureResource& m_textureResource;
		AudioManager& m_audioManager;
		Vehicle::Config m_carConfig, m_bikeConfig, m_shipConfig;
		void m_LoadMaps();

		sf::Clock m_roundTimer, m_resetTimer;
		const float m_resetTime; //amount of time when alll vehicles are dead before deciding id there's a round winner
		sf::Vector2f m_resetTarget; //camera destination when resetting round
		bool m_raceOver;
		bool m_altRules; //true when only 2 players
		sf::Vector2f m_cameraDestination;
		const float m_roundEndDuration;
		sf::Text m_debugText;

		//assign the current set of game rules to this
		std::function<void(float, tmx::MapLoader&)> m_UpdateGameRules;
		void m_UpdateLocalMulti(float dt, tmx::MapLoader& map);
		void m_UpdateTimeTrial(float dt, tmx::MapLoader& map);
		void m_UpdateSplitScreen(float dt, tmx::MapLoader& map);

		sf::Clock m_lapTimer;
		bool m_prevRaceStarted;

		//players are eliminated when leaving this area
		//creates a constant between screens of different ratios
		sf::FloatRect m_playerBounds;
		void m_SetPlayerBounds(const sf::Vector2f& centre);
		//resets all active vehicles
		void m_ResetAll(const Vehicle* leader);
		//moves the camera to given target and returns true when reached
		bool m_UpdateCamera(float dt, const sf::Vector2f& dest, sf::Uint8 viewId = 0u);

		//particles for sfx when player wins a round
		ParticleSystem m_starSystem;
		void m_StartCelebrating(sf::Uint8 vehicleId);

		//vector of track navigation nodes
		std::vector<TrackNode> m_trackNodes;
		//vector of vehicles
		std::vector< std::unique_ptr<Vehicle> >m_vehicles;
		//references to shared data
		SharedData::GameData& m_gameData;
		SharedData::MapData& m_mapData;
		SharedData::ScreenData& m_screenData;

		//adds a blur trail effect
		//buffer sprite is semi transparent and writes to the active buffer
		//buffer output sprite draws the output to the main window in Draw()
		//sf::Sprite m_bufferSprite, m_bufferOutputSprite;
		//sf::RenderTexture m_trailBufferA, m_trailBufferB; //these are swapped between buffer sprites
		//sf::Uint8 m_currentBuffer, m_trailLength;
		//const sf::Uint8 m_trailScale;
		//void m_UpdateTrail();

		//for debug drawing
		sf::VertexArray m_nodeLines, m_vehicleLines;
	};
};

#endif