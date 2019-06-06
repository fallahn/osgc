///source for hud manager - updates all hud elements and draws them to screen///
#include <Game/Hud.h>
#include <Game/Shaders/neonBlend.h>
#include <sstream>

using namespace Game;

//ctor
HudManager::HudManager(SharedData& sharedData)
	: m_gameData		(sharedData.Game),
	m_mapData			(sharedData.Map),
	m_screenData		(sharedData.Screen),
	m_lapMeter			(sharedData.Game.Players),
	m_drawHud			(true),
	m_hudFont			(sharedData.Fonts.Get("assets/fonts/default.ttf")),
	m_showScores		(false),
	m_showSummary		(false),
	m_scoreText			("this is a drill this is a drill", m_hudFont, 30u),
	m_nameText			(m_scoreText),
	m_winsText			(m_scoreText),
	m_raceTimes			("this is a drill\nthis is a drill", sharedData.Fonts.Get("assets/fonts/default.ttf"), 30u),
	m_prevRoundState	(Racing),
	m_suddenDeath		(false),
	m_flashCount		(0u),
	m_startLights		(sharedData.Textures.Get("assets/textures/ui/lights.png")),
	m_audioManager		(sharedData.AudioManager),
	m_vignetteTex		(sharedData.Textures.Get("assets/textures/ui/vignette.png"))
{
	//leaderboard
	m_leaderboardSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/leaderboard.png"));
	sf::Vector2u size = m_leaderboardSprite.getTexture()->getSize();
	m_leaderboardSprite.setOrigin(static_cast<sf::Vector2f>(size / 2u));

	//podium
	m_podiumSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/podium.png"));
	size = m_podiumSprite.getTexture()->getSize();
	m_podiumSprite.setOrigin(static_cast<sf::Vector2f>(size / 2u));

	m_podiumBackgroundSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/podium_background.png"));
	m_podiumBackgroundSprite.setOrigin(m_podiumSprite.getOrigin());

	for(auto& t : m_podiumText)
	{
		t.setFont(m_hudFont);
		t.setCharacterSize(45u);
	}

	m_neonShader.loadFromMemory(neonShader, sf::Shader::Fragment);
	m_shipSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/menu_ship_diffuse.png"), true);
	m_bikeSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/menu_bike_diffuse.png"), true);
	m_carSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/menu_buggy_diffuse.png"), true);

	m_bikeNeon = sharedData.Textures.Get("assets/textures/ui/menu_bike_neon.png");
	m_carNeon = sharedData.Textures.Get("assets/textures/ui/menu_buggy_neon.png");
	m_shipNeon = sharedData.Textures.Get("assets/textures/ui/menu_ship_neon.png");

	sf::Uint16 w, h;
	w = m_shipSprite.getTexture()->getSize().x;
	h = m_shipSprite.getTexture()->getSize().y;

	m_firstTexture.create(w, h);
	m_secondTexture.create(w, h);
	m_thirdTexture.create(w, h);

	h /= 12u; //magic number. 12 frames in this particular anim
	sf::Vector2f origin(static_cast<float>(w / 2u), static_cast<float>(h / 2u));

	m_firstSprite = AniSprite(m_firstTexture.getTexture(), w, h);
	m_firstSprite.setOrigin(origin);
	m_firstSprite.setFrameRate(10.f);
	m_firstSprite.play();
	m_secondSprite = AniSprite(m_secondTexture.getTexture(), w, h);
	m_secondSprite.setOrigin(origin);
	m_secondSprite.setFrameRate(10.f);
	m_secondSprite. play();
	m_thirdSprite = AniSprite(m_thirdTexture.getTexture(), w, h);
	m_thirdSprite.setOrigin(origin);
	m_thirdSprite.setFrameRate(12.f);
	m_thirdSprite.play();


	//vignette
	m_vignetteSprite.setTexture(m_vignetteTex, true);
}

//dtor


//public
void HudManager::Create()
{
	//Console::lout << "Building HUD..." << std::endl;

	m_startLights.Reset();

	//spacing for HUD elements
	const float rightEdge = m_screenData.HudView.getCenter().x - (m_screenData.HudView.getSize().x / 2.f);
	const float topEdge = m_screenData.HudView.getCenter(). y - (m_screenData.HudView.getSize().y / 2.f);
	const float padding = 6.f;

	//clear out any old objects
	m_scoreItems.clear();
	m_dualScore.clear();
	for(auto& t : m_podiumText)
		t.setString("");

	if(m_gameData.Rules == GameRules::Elimination)
	{	
		//create score items
		if(m_gameData.Players.size() > 2)
		{
			for(auto i = 0u; i < m_gameData.Players.size(); i++)
			{		
				m_scoreItems.push_back(std::unique_ptr<HudItemScore>(new HudItemScore(i, m_gameData.Players[i], m_gameData.RoundsToWin)));
				sf::Vector2f position;
				switch(i)
				{
				default:
				case 0:
					position = sf::Vector2f(rightEdge + padding, topEdge + padding);
					break;
				case 1:
					position = sf::Vector2f(m_screenData.HudView.getSize().x + rightEdge - (padding + m_scoreItems.back()->GetSize().x), topEdge + padding);
					break;
				case 2:
					position = sf::Vector2f(rightEdge + padding,
						topEdge + m_screenData.HudView.getSize().y - (padding + m_scoreItems.back()->GetSize().x + m_scoreItems.back()->GetSize().y));
					break;
				case 3:
					position = sf::Vector2f(m_screenData.HudView.getSize().x + rightEdge - (padding + m_scoreItems.back()->GetSize().x),
						topEdge + m_screenData.HudView.getSize().y - (padding + m_scoreItems.back()->GetSize().x + m_scoreItems.back()->GetSize().y));
					break;
				}
				m_scoreItems.back()->SetPosition(position);
			}
		}
		else //create single dual score
		{
			m_dualScore.push_back(std::unique_ptr<HudItemDualScore>(new HudItemDualScore(m_gameData)));
			m_dualScore.back()->SetPosition(sf::Vector2f(rightEdge + padding, topEdge + padding));
		}
	}
	//create lap meter
	m_lapMeter.Create(m_screenData.HudView.getSize().x * 0.9f, m_mapData.LapLength, m_gameData.MaxLaps);
	m_lapMeter.SetPosition(m_screenData.HudView.getCenter() + sf::Vector2f(0, (m_screenData.HudView.getSize().y / 2.f) - 30.f));
	m_lapMeter.SetColour(sf::Color::White);

	//position time trial text
	m_raceTimes.setPosition(rightEdge + padding, topEdge + padding);

	//create split screen bars
	if(m_gameData.Rules == GameRules::SplitScreen)
	{
		m_screenBars.Create(m_gameData.Players.size(), m_screenData.ViewSize);
		m_screenBars.Move(m_screenData.HudView.getCenter() - (m_screenData.HudView.getSize() / 2.f));
	}

	//reset any states
	m_suddenDeath = false;
	m_flashCount = 0u;
	m_showScores = false;
	m_showSummary = false;
	m_drawHud = true;

	//do this here after HUD view initialised
	m_leaderboardSprite.setPosition(m_screenData.HudView.getCenter());
	m_nameText.setPosition(m_leaderboardSprite.getPosition() - sf::Vector2f(360.f, 170.f));
	m_winsText.setPosition(m_nameText.getPosition() + sf::Vector2f(280.f, 0.f));
	m_scoreText.setPosition(m_winsText.getPosition() + sf::Vector2f(280.f, 0.f));

	//copy pointers to member vector so we don't sort global data
	m_scores.clear();
	for(const auto& p : m_gameData.Players)
		m_scores.push_back(p);

	//scale vignette to view size and position it
	m_vignetteSprite.setScale(m_screenData.DefaultView.getSize().x / m_vignetteTex.getSize().x,
						m_screenData.DefaultView.getSize().y / m_vignetteTex.getSize().y);
	m_vignetteSprite.setPosition(m_screenData.HudView.getCenter() - (m_screenData.HudView.getSize() / 2.f));
}

void HudManager::Update(float dt)
{		
	if(m_showSummary)
	{
		m_firstSprite.update();
		m_secondSprite.update();
		m_thirdSprite.update();
		return;
	}

	//update count in lights
	if(!m_startLights.Active() && !m_gameData.RaceStarted)
	{
		m_startLights.StartCounting(m_screenData.HudView);
		if(!m_startLights.Finished())
		{
			m_audioManager.PlayEffect(Audio::Start);
			m_audioManager.PlayMusic(static_cast<Audio::Music>(Helpers::Random::Int(1, 5)));
		}
	}
	m_gameData.RaceStarted = m_startLights.Update(dt, m_screenData.HudView);
	
	//update score items
	for(auto&& item : m_scoreItems) item->Update(dt);
	for(auto&& item : m_dualScore) item->Update(dt);

	//update lap meter
	m_lapMeter.Update(dt);
	m_lapMeter.SetLapCount(m_gameData.CurrentLap);

	//flash hud if sudden death
	if(m_flashCount && m_flashClock.getElapsedTime().asSeconds() > 0.5f)
	{
		m_flashClock.restart();
		m_drawHud = !m_drawHud;
		m_flashCount--;
	}

	//track roundstate and update messages based on changes
	if(m_gameData.CurrentRoundState != m_prevRoundState)
	{
		//reset any flashing icons
		for(auto&& s : m_scoreItems) s->Flash(false);
		for(auto&& d : m_dualScore) d->Flash(false);

		switch(m_gameData.CurrentRoundState)
		{
		case RaceWon:
			m_BuildScoreString();
			m_showScores = true;
			break;
		case RoundEnd:
			//flash icon
			if(!m_scoreItems.empty())
				m_scoreItems[m_gameData.WinnerId]->Flash(true);
			else if(!m_dualScore.empty())
				m_dualScore.back()->Flash(true);
			break;
		default: break;
		}
	}
	m_prevRoundState = m_gameData.CurrentRoundState;

	//track if sudden death and show message if changed
	if(m_gameData.SuddenDeath != m_suddenDeath)
	{
		m_lapMeter.SetColour(sf::Color::Red);
		m_flashCount = 6u; // must be even number
		m_flashClock.restart();
		m_audioManager.PlayEffect(Audio::SuddenDeath, m_screenData.RaceViews[0].getCenter());
	}

	m_suddenDeath = m_gameData.SuddenDeath;


	//update lap times
	if(m_gameData.Rules == GameRules::TimeTrial)
	{
		m_raceTimes.setString("Lap: " + m_TimeAsString(m_gameData.CurrentLapTime) 
			+ "\nFastest: " + m_TimeAsString(m_gameData.BestLapTime));
	}
}

void HudManager::DrawMessages(sf::RenderTarget& rt)
{
	if(m_drawHud &&
		m_gameData.Rules == GameRules::TimeTrial)
		rt.draw(m_raceTimes);
	
	if(m_showScores)
	{
		rt.draw(m_leaderboardSprite);
		rt.draw(m_nameText);
		rt.draw(m_winsText);
		rt.draw(m_scoreText);
	}
	else if(m_showSummary)
	{
		rt.draw(m_podiumSprite);
		for(const auto& t : m_podiumText)
			rt.draw(t);
	}
}

void HudManager::Summarise()
{
	//build summary menu from data
	m_podiumSprite.setPosition(m_leaderboardSprite.getPosition());
	m_podiumBackgroundSprite.setPosition(m_podiumSprite.getPosition());

	for(auto i = 0u; i < m_scores.size(); ++i) //there could be fewer scores than texts
		m_podiumText[i].setString(m_scores[i]->Name);

	//-----------TODO fix all magic numbers by tying in with texture sizes-------------------//

	//position each text
	sf::Vector2f centre;
	m_podiumText[0].setPosition(m_podiumSprite.getPosition() + sf::Vector2f(0.f, -200.f));
	m_podiumText[0].setOrigin(m_podiumText[0].getLocalBounds().width / 2.f, m_podiumText[0].getLocalBounds().height / 2.f);
	m_podiumText[1].setPosition(m_podiumSprite.getPosition() + sf::Vector2f(280.f, -160.f));
	m_podiumText[1].setOrigin(m_podiumText[1].getLocalBounds().width / 2.f, m_podiumText[1].getLocalBounds().height / 2.f);
	m_podiumText[2].setPosition(m_podiumSprite.getPosition() + sf::Vector2f(-280.f, -120.f));
	m_podiumText[2].setOrigin(m_podiumText[2].getLocalBounds().width / 2.f, m_podiumText[2].getLocalBounds().height / 2.f);

	m_firstSprite.setPosition(m_podiumSprite.getPosition() + sf::Vector2f(0.f, -80.f));
	m_secondSprite.setPosition(m_firstSprite.getPosition() + sf::Vector2f(280.f, 79.f));
	m_thirdSprite.setPosition(m_firstSprite.getPosition() + sf::Vector2f(-280.f, 158.f));	
		
	//render all to texture
	m_UpdatePodiumTexture(m_scores[0].get(), m_firstTexture);
	m_UpdatePodiumTexture(m_scores[1].get(), m_secondTexture);

	if(m_gameData.Players.size() > 2)
	{
		m_UpdatePodiumTexture(m_scores[2].get(), m_thirdTexture);
		m_thirdSprite.setColor(sf::Color::White);
	}
	//hide last sprite if necessary
	else
	{
		m_thirdSprite.setColor(sf::Color(255u, 255u, 255u, 0u));
	}

	//and set to display
	m_showScores = false;
	m_drawHud = false;
	m_showSummary = true;
}

//private
void HudManager::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	rt.draw(m_vignetteSprite);	
	
	//split screen bars
	if(m_gameData.Rules == GameRules::SplitScreen)
		rt.draw(m_screenBars);

	//draw lights
	if(m_startLights.Active()) rt.draw(m_startLights.GetSprite());	

	if(m_drawHud)
	{
		//draw score items
		for(const auto& scoreItem : m_scoreItems)
			rt.draw(scoreItem->GetSprite());

		for(const auto& dualScore : m_dualScore)
			rt.draw(dualScore->GetSprite());

		//draw track meter
		rt.draw(m_lapMeter.GetSprite());

		//TODO draw other items

	}
	else if(m_showSummary)
	{
		rt.draw(m_podiumBackgroundSprite);
		
		//draw podium vehicles
		rt.draw(m_firstSprite);
		rt.draw(m_secondSprite);
		rt.draw(m_thirdSprite);
	}
}

void HudManager::m_UpdatePodiumTexture(const PlayerData* data, sf::RenderTexture& rt)
{
	m_neonShader.setParameter("neonColour", PlayerColours::Light(data->ID));
	rt.clear(sf::Color::Transparent);
	switch (data->Vehicle)
	{
	default:
	case Vehicle::Bike:
		m_neonShader.setParameter("neonMap", m_bikeNeon);
		rt.draw(m_bikeSprite, &m_neonShader);
		break;
	case Vehicle::Buggy:
		m_neonShader.setParameter("neonMap", m_carNeon);
		rt.draw(m_carSprite, &m_neonShader);
		break;
	case Vehicle::Ship:
		m_neonShader.setParameter("neonMap", m_shipNeon);
		rt.draw(m_shipSprite, &m_neonShader);
		break;
	}
	rt.display();
}

void HudManager::m_BuildScoreString()
{
	if(m_gameData.Rules == GameRules::TimeTrial) //TODO load top ten from local or network resource
	{
		m_nameText.setString("Name:\n\n" + m_gameData.Players[0]->Name);
		m_winsText.setString("Best Time:\n\n" + m_TimeAsString(m_gameData.BestLapTime));
		switch(m_gameData.Players[0]->Vehicle)
		{
		case Vehicle::Bike:
			m_scoreText.setString("Vehicle:\n\nBike");
			break;
		case Vehicle::Buggy:
			m_scoreText.setString("Vehicle:\n\nBuggy");
			break;
		case Vehicle::Ship:
			m_scoreText.setString("Vehicle:\n\nShip");
			break;
			default: break;
		}
	}
	else
	{
		//sort by score, sub sorted by wins
		std::sort(m_scores.begin(),
			m_scores.end(),
			[](const std::shared_ptr<PlayerData>& a, const std::shared_ptr<PlayerData>& b)->bool
		{
			if(a->Score > b->Score) return true;
			if(a->Score == b->Score) return (a->RaceWins > b->RaceWins);
			return false;
		});


		//-----build actual strings-----//
		//leaderboard
		sf::Uint8 racePos = m_scores.size(); //reposition spawn for next race
		std::stringstream ns, rs, ss;
		for(auto& s : m_scores)
		{
			racePos--;
			s->GridPosition = racePos;
		
			ns << s->Name << std::endl;
			rs << static_cast<short>(s->RaceWins) << std::endl;
			ss << static_cast<short>(s->Score) << std::endl;
		}

		m_nameText.setString("Name:\n\n" + ns.str());
		m_winsText.setString("Wins:\n\n" + rs.str());
		m_scoreText.setString("Score:\n\n" + ss.str());
	}
}

std::string HudManager::m_TimeAsString(float time)
{
	if(time > 999) return "-:---";
	
	float minutes = std::floor(time / 60.f);
	float seconds = time - (minutes * 60.f);
	std::stringstream ss;
	ss << minutes << "m" << seconds << "s";
	return ss.str();
}