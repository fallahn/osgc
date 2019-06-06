//***header for HUD item classes***//
#ifndef HUD_H_
#define HUD_H_

#include <Game/SharedData.h>

#include <SFML/Graphics/CircleShape.hpp>

#include <memory>

namespace Game
{
	//-------------------------Manager--------------------------//
	//manages all the active elements during game play
	class HudManager : private sf::NonCopyable, public sf::Drawable
	{
	public:
		HudManager(SharedData& sharedData);

		void Create();
		void Update(float dt); //use the view to set item positions
		void DrawMessages(sf::RenderTarget& rt); //draw messages separate from bloom effect
		void Summarise(); //creates summary display with race results
	private:
		//shape class for score items
		class ScoreShape : public sf::Drawable
		{
		public:
			ScoreShape(sf::Color colour, float radius);
			void setPosition(float x, float y);
			void setPosition(const sf::Vector2f& position);
			void move(float x, float y);
			const sf::Vector2f getPosition() const{return m_bodyShape.getPosition();};
		private:
			sf::CircleShape m_bodyShape, m_highlightShape, m_ringShape;
			bool m_drawRing;
			void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		};


		//--------------------Base class for HUD Items---------------//
		class HudItemBase : private sf::NonCopyable
		{
		public:
			HudItemBase(){};
			virtual ~HudItemBase(){};
			virtual void Update(float dt) = 0;
			const sf::Sprite& GetSprite() const{return m_sprite;};
			void SetPosition(const sf::Vector2f& position){m_sprite.setPosition(position);};
		
		protected:
			sf::RenderTexture m_renderTexture;
			sf::Sprite m_sprite;
		};

		//---------------------Lap distance item---------------------//
		class HudItemLapMeter : public HudItemBase
		{
		public:
			HudItemLapMeter(const std::vector< std::shared_ptr<PlayerData> >& playerData);

			void Update(float dt);
			void Create(float screenWidth, float trackLength, sf::Uint8 totalLaps);
			void SetColour(const sf::Color& colour){m_trackbar.setOutlineColor(colour);};
			void SetLapCount(sf::Uint8 lapCount){m_lapCount = lapCount;};
		private:
			const std::vector< std::shared_ptr<PlayerData> >& m_playerData;
			sf::RectangleShape m_trackbar, m_lapBar;
			std::vector<sf::CircleShape> m_playerShapes;
			sf::CircleShape m_highlight;
			float m_scale, m_barWidth, m_lapLength;
			sf::Uint8 m_lapCount;
		}m_lapMeter;

		//------------------------Score Item-------------------------//
		class HudItemScore : public HudItemBase
		{
		public:
			HudItemScore(sf::Uint8 playerId, const std::shared_ptr<PlayerData>& playerData, sf::Uint8 size = 6u);
			void Update(float dt);
			sf::Vector2f GetSize() const
			{return static_cast<sf::Vector2f>(m_renderTexture.getSize());};
			void Flash(bool b){m_flashIcon = b;};
		private:
			ScoreShape m_lightShape, m_darkShape;
			sf::Uint8 m_size;
			float m_iconSpacing;
			const std::shared_ptr<PlayerData>& m_data;
			bool m_flashIcon;
			bool m_flashOn;
			sf::Clock m_flashClock;
			const float m_flashTime;
		};
		//for 2 player scores - TODO many common members could be derived from same base class
		class HudItemDualScore : public HudItemBase
		{
		public:
			HudItemDualScore(const SharedData::GameData& gameData, sf::Uint8 size = 8u);
			void Update(float dt);
			void Flash(bool b){m_flashIcon = b;};
		private:
			sf::Uint8 m_size;
			float m_iconSpacing;
			const SharedData::GameData& m_data;
			ScoreShape m_shapeRed;
			ScoreShape m_shapeBlue;
			bool m_flashIcon;
			bool m_flashOn;
			sf::Clock m_flashClock;
			const float m_flashTime;
		};
		//----------------------Start Lights-------------------------//
		class HudItemStartLights : private sf::NonCopyable
		{
		public:
			HudItemStartLights(const sf::Texture& texture);
			bool Update(float dt, const sf::View& hudView);
			const sf::Sprite& GetSprite() const{return m_sprite;};
			bool Active() const{return m_active;};
			void StartCounting(const sf::View& hudView);
			void Reset(){m_finished = false;};
			bool Finished() const{return m_finished;};
		private:
			const sf::Texture& m_texture;
			AniSprite m_sprite;
			bool m_active, m_finished;
			float m_endY; //value of final position of sprite
		}m_startLights;
		//----------------Black bars on split screen----------------//
		class HudItemScreenBars : public sf::Drawable, private sf::NonCopyable
		{
		public:
			HudItemScreenBars();
			void Create(sf::Uint8 screenCount, const sf::Vector2f& screenSize);
			void Move(const sf::Vector2f& distance);
		private:
			sf::VertexArray m_vertices;
			void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		}m_screenBars;

		//items which make up HUD
		std::vector< std::unique_ptr<HudItemScore> > m_scoreItems;
		std::vector< std::unique_ptr<HudItemDualScore> > m_dualScore;

		//references to shared data
		SharedData::MapData& m_mapData;
		SharedData::GameData& m_gameData;
		const SharedData::ScreenData& m_screenData;

		sf::Clock m_flashClock;
		bool m_drawHud;

		//used to track scores for scoreboard and podium
		std::vector< std::shared_ptr<PlayerData> > m_scores;

		//displays the time in time trial
		sf::Text m_raceTimes;

		//displays leaderboard / podium
		sf::Font m_hudFont;
		sf::Text m_scoreText, m_nameText, m_winsText;
		std::array<sf::Text, 4> m_podiumText;
		bool m_showScores;
		bool m_showSummary;
		void m_BuildScoreString();
		std::string m_TimeAsString(float time);
		sf::Sprite m_leaderboardSprite, m_podiumSprite;
		sf::Sprite m_podiumBackgroundSprite;

		//for observing roundstate
		RoundState m_prevRoundState;
		bool m_suddenDeath;
		sf::Uint8 m_flashCount;

		AudioManager& m_audioManager; //reference to audio manager

		//vignette
		const sf::Texture& m_vignetteTex;
		sf::Sprite m_vignetteSprite;

		//vehicle sprites for podium
		AniSprite m_firstSprite, m_secondSprite, m_thirdSprite;
		sf::RenderTexture m_firstTexture, m_secondTexture, m_thirdTexture;
		sf::Sprite m_shipSprite, m_bikeSprite, m_carSprite;
		sf::Texture m_shipNeon, m_bikeNeon, m_carNeon;
		sf::Shader m_neonShader;

		//drawable overload
		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		void m_UpdatePodiumTexture(const PlayerData* data, sf::RenderTexture& rt);
	};
};

#endif //HUD_H_