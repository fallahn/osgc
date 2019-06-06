#include <Game/GameScreen.h>
#include <Game/Shaders/Displacement.h>
#include <Game/Shaders/Saturate.h>

using namespace Game;

GameScreen::GameScreen(sf::RenderWindow& renderWindow, SharedData& sharedData)
	: BaseScreen			(renderWindow, sharedData),
	m_map(tmx::MapLoader	("assets/maps")),
	m_vehicleManager		(sharedData),
	m_hudManager			(sharedData),
	m_fpsSampleRate			(10.f),
	m_showSummary			(false),
	m_asteroidField			(sharedData.Textures),
	m_starField				(sharedData.Textures),
	m_lastVolume			(0.f) //stores volume when muting during pause
{
	m_displacementShader.loadFromMemory(displacment, sf::Shader::Fragment);
	m_saturateShader.loadFromMemory(saturate, sf::Shader::Fragment);
	
}

GameScreen::~GameScreen()
{

}

void GameScreen::m_HandleCustomEvent()
{
	if(m_event.type == sf::Event::KeyReleased
		&& m_hasFocus)
	{
		switch(m_event.key.code)
		{
		case sf::Keyboard::Return:
			if(m_showSummary)
			{
				m_showSummary = false;
				m_return = MAIN_MENU;
				m_running = false;
			}
			break;
		case sf::Keyboard::Space:
			if(m_sharedData.Game.CurrentRoundState == RaceWon)
			{
				m_vehicleManager.EndRace();
			}
			break;
		case sf::Keyboard::Escape:
			m_return = MAIN_MENU;
			m_running = false;
			break;
		case sf::Keyboard::BackSpace:
			m_return = QUIT;
			m_running = false;
			break;
		case sf::Keyboard::P:
			if(m_timeScale < 1)
			{
				m_timeScale = 1.f;
				m_sharedData.AudioManager.SetGlobalEffectsVolume(m_lastVolume);
			}
			else
			{
				m_timeScale = 0.f;
				m_lastVolume = m_sharedData.AudioManager.GetGlobalEffectsVolume();
				m_sharedData.AudioManager.SetGlobalEffectsVolume(0.f);
				m_Update(0.f); //do one single update so volume changes are propegated
				//std::cout << m_sharedData.AudioManager.GetGlobalEffectsVolume() << std::endl;
			}
			break;
		case sf::Keyboard::Subtract:
			if(m_timeScale > 0.f) m_timeScale -= 0.1f;
			//Console::lout << "Set timescale to: " << m_timeScale << std::endl;
			break;
		case sf::Keyboard::Add:
			if(m_timeScale < 1.f) m_timeScale += 0.1f;
			//Console::lout << "Set timescale to: " << m_timeScale << std::endl;
			break;
		default: break;
		}
	}
}

void GameScreen::m_Update(float dt)
{
	//////////////////////////////
	//**update game logic here**//
	//////////////////////////////
	
	m_viewRect = Helpers::Rectangles::GetViewRect(m_sharedData.Screen.RaceViews[0]);

	//must update quad tree at least once before trying to query it
	//m_map.UpdateQuadTree(m_viewRect); //moved to vehicle manager
	m_vehicleManager.Update(dt, m_map);
	//end race if someone won
	if(m_vehicleManager.RaceOver() && !m_showSummary)
	{
		if(m_sharedData.PlayList.size())
			m_sharedData.PlayList.pop_front();
		m_sharedData.Game.Reset();

		if(!m_sharedData.PlayList.empty())
		{
			m_vehicleManager.Reset();
			m_energyFences.clear();
					
			std::string msg = "Loading: " + m_sharedData.PlayList.front().MapName;
			if(m_sharedData.PlayList.front().Reverse) msg += " (Reverse)";
			
			m_LaunchLoadingWindow(msg);
			m_LoadNextMap(); //TODO handle loading failure
			m_StopLoadingWindow();
		}
		else 
		{
			if(m_sharedData.Game.Rules != GameRules::TimeTrial)
			{
				m_hudManager.Summarise(); //shows the podium
				m_showSummary = true;
				//return; //prevent resetting fences and vehicles until game quit
			}
			else
			{
				//return to menu
				m_return = MAIN_MENU;
				m_running = false;
			}
		}

	}
	//update this because its likely to have moved
	m_viewRect = Helpers::Rectangles::GetViewRect(m_sharedData.Screen.RaceViews[0]);

	//update background and track velocity of game view by comparing position with last position
	sf::Vector2f viewPos = m_sharedData.Screen.RaceViews[0].getCenter();
	sf::Vector2f starVelocity = Helpers::Vectors::Normalize(m_lastViewPosition - viewPos);
	m_starField.Update(dt, sf::FloatRect(sf::Vector2f(), m_sharedData.Screen.ViewSize), starVelocity);
	m_lastViewPosition = viewPos;

	//update the HUD
	m_hudManager.Update(dt);

	//update roids
	//sf::Vector2f lightPos = m_sharedData.Map.LightPosition;// -sf::Vector2f(m_viewRect.left, m_viewRect.top + m_viewRect.height);
	//lightPos *= m_sharedData.Screen.WindowScale; //TODO this should def be in frag coords of current window size

	if(m_map.QuadTreeAvailable())
		m_asteroidField.Update(dt, m_sharedData.Map.LightPosition, m_vehicleManager.GetVehicles(), m_map);

	//update electric fences
	for(auto&& e : m_energyFences) e->Update(dt);

	//update audio listener to follow screen
	if(m_sharedData.Game.Rules != GameRules::SplitScreen)
		m_sharedData.AudioManager.SetListenerPosition(viewPos);
	m_sharedData.AudioManager.RemoveStoppedSounds();

	//update 3d stuff
	m_meshScene->UpdateScene(dt, m_sharedData.Screen.RaceViews);
}

void GameScreen::m_Render()
{
	////////////////////////////
	//**draw game items here**//
	////////////////////////////


	//------draw starfield to its own buffer------//
	sf::View starView(m_sharedData.Screen.ViewSize / 2.f, m_sharedData.Screen.ViewSize);
	//correct aspect for 2 player
	if(m_sharedData.Screen.RaceViews.size() == 2)
	{
		sf::Vector2f s = m_sharedData.Screen.ViewSize;
		s.x /= 2.f;
		starView.setSize(s);
	}

	m_starBuffer.clear(sf::Color::Transparent);
	for(const auto& v : m_sharedData.Screen.RaceViews)
	{
		starView.setViewport(v.getViewport());
		m_starBuffer.setView(starView);
		m_starBuffer.draw(m_starField);
	}
	m_starBuffer.display();


	//----draw track normal to use with star field---//
	m_normalBuffer.clear(sf::Color(128u, 128u, 255u)); //flat normal colour
	for(const auto& v : m_sharedData.Screen.RaceViews)
	{
		m_normalBuffer.setView(v);
		m_map.Draw(m_normalBuffer, 1);
	}
	//m_vehicleManager.Draw(m_normalBuffer,false);
	m_normalBuffer.display();


	//----draw to extract buffer-----
	m_extractBuffer.clear(sf::Color::Transparent);
	m_extractBuffer.setView(m_sharedData.Screen.DefaultView);
	m_extractBuffer.draw(m_starSprite, &m_displacementShader);

	for(const auto& v : m_sharedData.Screen.RaceViews)
	{
		m_extractBuffer.setView(v);
		m_map.Draw(m_extractBuffer, 3); //neon layer
		m_vehicleManager.Draw(m_extractBuffer, false);

		sf::FloatRect cull = Helpers::Rectangles::GetViewRect(v);
		for(auto&& e : m_energyFences)
		{
			//cull to viewable area
			if(cull.contains(e->GetEnd()) ||
				cull.contains(e->GetStart()))
					m_extractBuffer.draw(*e);
		}

		m_extractBuffer.draw(m_asteroidField); //TODO alpha mask shader
	}
	m_meshScene->DrawSelfIllum(m_extractBuffer);
	m_extractBuffer.setView(m_sharedData.Screen.HudView);
	m_extractBuffer.draw(m_hudManager);
	m_extractBuffer.display();
	
	//----scale and extract to blur buffer----
	m_extractSprite.setScale(m_blurmapScale, m_blurmapScale);
	m_verticalBuffer.clear(sf::Color::Transparent);
	m_verticalBuffer.draw(m_extractSprite, &m_sharedData.Screen.Shaders.BloomExtract);
	m_verticalBuffer.display();
	m_extractSprite.setScale(1.f, 1.f);

	m_DoPostBlur();
	
	//----draw result to main window----
	m_renderWindow.clear(sf::Color::Transparent);
	m_renderWindow.pushGLStates();
	m_renderWindow.setView(m_sharedData.Screen.DefaultView);
	m_renderWindow.draw(m_starSprite, &m_displacementShader);

	for(const auto& v : m_sharedData.Screen.RaceViews)
	{
		m_renderWindow.setView(v);
		m_vehicleManager.Draw(m_renderWindow, true);
		m_map.Draw(m_renderWindow, 0); //Track
		m_renderWindow.draw(m_shadowSprite, sf::BlendMultiply);
		m_map.Draw(m_renderWindow, 2); //detail
		m_map.Draw(m_renderWindow, 3); //neon
	
		m_vehicleManager.Draw(m_renderWindow, false);

		sf::FloatRect cull = Helpers::Rectangles::GetViewRect(v);
		for(auto&& e : m_energyFences)
		{
			if(cull.contains(e->GetEnd()) ||
				cull.contains(e->GetStart()))
					m_renderWindow.draw(*e);
		}
		m_renderWindow.draw(m_asteroidField);
	}


	//debug text
	//m_map.Draw(m_renderWindow, tmx::MapLayer::Debug);
	//m_vehicleManager.DrawDebug(m_renderWindow);

	//draw 3d sprite
	m_renderWindow.draw(*m_meshScene);
	//m_meshScene->DrawSelfIllum(m_renderWindow);

	//m_renderWindow.draw(sf::Sprite(m_normalBuffer.getTexture()));
	//draw hud
	m_renderWindow.setView(m_sharedData.Screen.HudView);
	m_renderWindow.draw(m_hudManager);
	
	//draw post effect (blur) and non-blurred messages
	m_renderWindow.draw(m_postSprite, m_postStates);
	m_hudManager.DrawMessages(m_renderWindow);

	m_renderWindow.popGLStates();
	m_renderWindow.display();
	
	//log fps to get an average
	const float fr = m_DrawFps();
	if(m_averageClock.getElapsedTime().asSeconds() > m_fpsSampleRate)
	{
		m_fpsReadings.push_back(fr);
		m_averageClock.restart();
	}
}

void GameScreen::m_OnStart()
{	
	m_renderWindow.setMouseCursorVisible(false);

	//create views based on game play mode
	m_sharedData.Screen.RaceViews.clear();
	if(m_sharedData.Game.Rules == GameRules::SplitScreen)
	{
		assert(m_sharedData.Game.Players.size() > 1u);
		switch(m_sharedData.Game.Players.size())
		{
		case 2:
			{
			sf::View v1(sf::Vector2f(), sf::Vector2f(m_sharedData.Screen.ViewSize.x * 0.6f, m_sharedData.Screen.ViewSize.y * 1.2f));
			sf::View v2 = v1;

			v1.setViewport(sf::FloatRect(0.f, 0.f, 0.5f, 1.f));
			v2.setViewport(sf::FloatRect(0.5f, 0.f, 0.5f, 1.f));

			m_sharedData.Screen.RaceViews.push_back(v1);
			m_sharedData.Screen.RaceViews.push_back(v2);
			break;
			}
		case 3:
		case 4:
			{
			sf::View v1(sf::Vector2f(), m_sharedData.Screen.ViewSize * 0.7f); //zoomed out slightly for split screen
			sf::View v2, v3, v4;
			v2 = v3 = v4 = v1;

			v1.setViewport(sf::FloatRect(0.f, 0.f, 0.5f, 0.5f));
			v2.setViewport(sf::FloatRect(0.5f, 0.f, 0.5f, 0.5f));
			v3.setViewport(sf::FloatRect(0.f, 0.5f, 0.5f, 0.5f));
			v4.setViewport(sf::FloatRect(0.5f, 0.5f, 0.5f, 0.5f));

			m_sharedData.Screen.RaceViews.push_back(v1);
			m_sharedData.Screen.RaceViews.push_back(v2);
			m_sharedData.Screen.RaceViews.push_back(v3);
			m_sharedData.Screen.RaceViews.push_back(v4);
			break;
			}
		default: break;
		}
	}
	else
	{
		m_sharedData.Screen.RaceViews.push_back(m_sharedData.Screen.DefaultView);
	}

	//load and create entities
	m_sharedData.Game.Reset();
	m_fpsReadings.clear();
	m_showSummary = false;

	//----create rendering buffers----//
	//sprite for rendering background to
	m_starBuffer.create(static_cast<unsigned>(m_sharedData.Screen.ViewSize.x), static_cast<unsigned>(m_sharedData.Screen.ViewSize.y));
	m_starSprite = sf::Sprite(m_starBuffer.getTexture());

	//buffer for track normal mapping
	m_normalBuffer.create(m_starBuffer.getSize().x / 4u, m_starBuffer.getSize().y / 4u); //scaled smaller as normal doesn't need to be accurate
	m_normalBuffer.setSmooth(true);
	m_displacementShader.setParameter("displaceTexture", m_normalBuffer.getTexture());


	//setup up HUD view
	m_sharedData.Screen.HudView = m_sharedData.Screen.DefaultView;
	m_sharedData.Screen.HudView.setCenter(-m_sharedData.Screen.ViewSize);

	//incase screen was resized recreate post sprite
	m_postSprite = sf::Sprite(m_verticalBuffer.getTexture()); 
	m_postSprite.setScale(1.f / m_blurmapScale, 1.f / m_blurmapScale);
	m_postSprite.setPosition(m_sharedData.Screen.HudView.getCenter() - (m_sharedData.Screen.HudView.getSize() / 2.f));
	//-------------------------------------------------------//

	while(!m_LoadNextMap() && !m_sharedData.PlayList.empty())
		m_sharedData.PlayList.pop_front();

	//set render states for post effect
	m_postStates.shader = &m_saturateShader;
	m_postStates.blendMode = sf::BlendAdd;

}

void GameScreen::m_OnFinish()
{
	//output average frame rate at end of round
	float avgFps = 0.f;
	for(const auto fps : m_fpsReadings)
		avgFps += fps;

	avgFps /= static_cast<float>(m_fpsReadings.size());
	std::cout << "Average frame rate: " << avgFps << std::endl;

	//clear any left over maps
	m_sharedData.PlayList.clear();
	//reset vehicles
	m_vehicleManager.Reset();
	//clear fences
	m_energyFences.clear();

	//stop any music we may have had
	m_sharedData.AudioManager.StopMusic();
}

void GameScreen::m_DoPostBlur()
{
	m_sharedData.Screen.Shaders.BloomBlur.setParameter("orientation", 0.f);
	m_horizontalBuffer.clear(sf::Color::Transparent);
	m_horizontalBuffer.draw(sf::Sprite(m_verticalBuffer.getTexture()), &m_sharedData.Screen.Shaders.BloomBlur);
	m_horizontalBuffer.display();

	m_sharedData.Screen.Shaders.BloomBlur.setParameter("orientation", 1.f);
	m_verticalBuffer.clear(sf::Color::Transparent);
	m_verticalBuffer.draw(sf::Sprite(m_horizontalBuffer.getTexture()), &m_sharedData.Screen.Shaders.BloomBlur);
	m_verticalBuffer.display();
}

bool GameScreen::m_LoadNextMap()
{
	if(m_sharedData.PlayList.empty()) return false;

	std::string mapName = m_sharedData.PlayList.front().MapName;

	if(m_map.Load(mapName + ".tmx"))
	{
		//assume light position is centre of map
		m_sharedData.Map.LightPosition.x = m_map.GetMapSize().x / 2.f;
		m_sharedData.Map.LightPosition.y = m_map.GetMapSize().y / 2.f;

		//set up the shadow map
		m_shadowMap = m_sharedData.Textures.Get("assets/maps/shadowmaps/" + mapName + ".png");
		m_shadowMap.setSmooth(true);
		m_shadowSprite.setTexture(m_shadowMap, true);
		float scale = static_cast<float>(m_map.GetMapSize().x / m_shadowMap.getSize().x);
		m_shadowSprite.setScale(scale, scale);

		//set asteroid field bounds
		m_asteroidField.SetBounds(static_cast<sf::Vector2f>(m_map.GetMapSize()));

		//parse map nodes
		for(auto layer = m_map.GetLayers().begin(); layer != m_map.GetLayers().end(); ++layer)
		{
			//create electric fences
			m_energyFences.clear();
			if(layer->name == "Fence")
			{
				for(auto& o : layer->objects)
				{
					//std::cout << "Found fence object" << std::endl;
					if(o.FirstPoint() != o.LastPoint())
					{
						m_energyFences.push_back(
							std::unique_ptr<EnergyFence>(
							new EnergyFence(o.FirstPoint(), o.LastPoint(), m_sharedData.Textures.Get("assets/textures/environment/electric.png"), m_sharedData.AudioManager
							)));
						//std::cout << "Created Fence object" << std::endl;
					}
				}
			}			
			//send waypoints to vehicle manager
			if(layer->name == "Waypoints")
			{
				m_vehicleManager.ParseMapNodes(*layer, m_sharedData.PlayList.front().Reverse);
				//break;
			}
		}

		//load 3D models
		m_meshScene.reset(new ml::MeshScene(m_renderWindow));
		m_meshScene->LoadScene(mapName);
		//set this paramter *after* creating scene node as texture will have been recreated
		m_displacementShader.setParameter("alphaMask", m_meshScene->GetTexture());

		//set number of laps
		m_sharedData.Game.MaxLaps = atoi(m_sharedData.PlayList.front().Laps.c_str());

		//create HUD - must do AFTER setting up Hud view (done in m_OnStart), and lap count ^^
		m_hudManager.Create();

		//if 3 player make 4th view into minimap
		if(m_sharedData.Game.Rules == GameRules::SplitScreen
			&& m_sharedData.Game.Players.size() == 3u)
		{
			sf::Vector2f mapSize = static_cast<sf::Vector2f>(m_map.GetMapSize());
			sf::Vector2f viewSize = m_sharedData.Screen.RaceViews.back().getSize();
			const float ratio = viewSize.x / viewSize.y;

			viewSize = mapSize;
			if(mapSize.x < mapSize.y)
			{
				viewSize.x = mapSize.y * ratio;
			}
			else
			{
				viewSize.y = mapSize.x / ratio;
				if(viewSize.y < mapSize.y)
				{
					float r = mapSize.y / viewSize.y;
					viewSize *= r;
				}
			}
			m_sharedData.Screen.RaceViews.back().setSize(viewSize * 1.05f);
		}

		//and create vehicles
		return m_vehicleManager.CreateVehicles();
	}
	else return false;
}