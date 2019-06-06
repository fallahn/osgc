///***defines a struct which is used to share data between different screen classes***///
#ifndef SHARED_DATA
#define SHARED_DATA

#include <Game/ResourceManager.h>
#include <Game/AudioManager.h>
#include <Game/Vehicle.h>
#include <vector>
#include <list>
#include <memory>

namespace Game
{
	struct PlayerData
	{
		PlayerData(std::string name, ControlSet controls, Vehicle::VehicleType vehicle, sf::Uint8 id, Intelligence intelligence = Human)
			: Name			(name),
			Controls		(controls),
			RoundWins		(0u),
			RaceWins		(0u),
			Score			(0u),
			GridPosition	(id),
			ID				(id),
			Vehicle			(vehicle),
			CpuIntelligence	(intelligence),
			Active			(true){};
		std::string Name;
		ControlSet Controls;
		sf::Uint8 RoundWins, RaceWins, Score, GridPosition, ID;
		Intelligence CpuIntelligence;
		Vehicle::VehicleType Vehicle;
		float CurrentDistance;
		bool Active;
	};
	
	enum class GameRules
	{
		Elimination,
		SplitScreen,
		TimeTrial
	};

	struct SharedData
	{
		TextureResource Textures;
		AudioManager AudioManager;
		FontResource Fonts;

		struct RaceInfo
		{
			RaceInfo(std::string name = "", std::string laps = "0", bool reverse = false)
				: Laps	(laps),
				MapName	(name),
				Reverse	(reverse){};
			std::string Laps;
			std::string MapName;
			bool Reverse;
		};
		std::list<std::string> MapList;
		std::list<RaceInfo> PlayList; //list of maps to play through
		
		struct GameData
		{
			GameData() 
				: Rules		(GameRules::Elimination),
				RaceStarted	(false),
				SuddenDeath	(false),
				MaxLaps		(4u),
				RoundsToWin	(4u),
				CurrentLap	(0u),
				WinnerId	(0u),
				CurrentRoundState(Racing),
				CurrentLapTime(0.f),
				BestLapTime	(1000.f){};
			GameRules Rules;
			std::vector< std::shared_ptr<PlayerData> > Players;
			bool RaceStarted;
			bool SuddenDeath;
			sf::Uint8 MaxLaps, RoundsToWin, CurrentLap;
			sf::Uint8 WinnerId; //ID of player who won last point
			RoundState CurrentRoundState;
			float CurrentLapTime, BestLapTime;
			//allows resetting of state between races
			void Reset()
			{
				RaceStarted = false;
				SuddenDeath = false;
				CurrentLap = 0u;
				CurrentRoundState = Racing;
				CurrentLapTime = 0.f;
				BestLapTime = 1000.f;
				for(auto& player : Players)
				{
					//different rules for 2 player
					player->RoundWins = (Players.size() < 3)? 4u : 0u;
				}
			}

		} Game;

		struct MapData
		{
			MapData() :	LapLength(0.f){};
			sf::Vector2f LightPosition;
			float LapLength; //length of a single lap

		} Map;

		struct ScreenData
		{
			ScreenData() : WindowScale (1.f){};

			sf::Vector2f ViewSize;
			sf::View DefaultView, HudView;
			std::vector<sf::View> RaceViews;
			float WindowScale; //window size compared to view size - useful for scaling shader resolutions which work in frag coords

			struct Shaders //TODO move to own resource manager
			{			
				sf::Shader BloomExtract;
				sf::Shader BloomBlur;
				sf::Shader BloomBlend;
			} Shaders;
		} Screen;
	};
};

#endif //SHARED_DATA