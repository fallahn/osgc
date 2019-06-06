///header file for game screen class
#ifndef GAMESCREEN_H_
#define GAMESCREEN_H_

#include <Game/BaseScreen.h>
#include <Game/AsteroidField.h>
#include <Game/EnergyFence.h>
#include <Game/VehicleManager.h>
#include <Game/Hud.h>

#include <tmx/MapLoader.h>

namespace Game
{
	//default game screen which runs/renders the game
	class GameScreen final : public BaseScreen
	{
	public:
		GameScreen(sf::RenderWindow& renderWindow, SharedData& sharedData);
		~GameScreen();
	private:
		void m_HandleCustomEvent();
		void m_Update(float dt);
		void m_Render();
		void m_OnStart();
		void m_OnFinish();
		void m_OnContextCreate(){};
		
		//game specific members
		tmx::MapLoader m_map;
		VehicleManager m_vehicleManager;
		HudManager m_hudManager;
		AsteroidField m_asteroidField;
		std::vector< std::unique_ptr<EnergyFence> >m_energyFences;

		bool m_showSummary;

		sf::Sprite m_postSprite; //used to render post buffer
		void m_DoPostBlur(); //ping pongs blur buffers
		bool m_LoadNextMap();
		
		//background stuff
		sf::Vector2f m_lastViewPosition;
		StarField m_starField;
		sf::RenderTexture m_starBuffer, m_normalBuffer;
		sf::Sprite m_starSprite;
		sf::Shader m_displacementShader;
		sf::Shader m_saturateShader;

		//sprite for shadow map
		sf::Sprite m_shadowSprite;
		sf::Texture m_shadowMap;

		//render / update optimising
		sf::FloatRect m_viewRect; //currently viewable area
		sf::RenderStates m_postStates; //declared here so we only need set them once

		//stores last volume for restoring on unpause
		float m_lastVolume;

		//to calc avg fps
		std::vector<float> m_fpsReadings;
		sf::Clock m_averageClock;
		const float m_fpsSampleRate;

		//3d stuffs
		std::unique_ptr<ml::MeshScene> m_meshScene;
		ml::MeshResource m_meshResource;
	};
}
#endif //GAMESCREEN_H_