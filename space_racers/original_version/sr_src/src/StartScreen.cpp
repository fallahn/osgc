///source code for default start up screen / main menu///
#include <Game/StartScreen.h>
#include <Game/UIButton.h>
#include <Game/Shaders/BlendOverlay.h>
#include <Game/SaveSettings.h>

using namespace Game;

namespace
{
	std::array<std::string, 21> names =
		{"Ralph",
		"Adriana",
		"Twiglet",
		"Spider",
		"Bennet",
		"Jonah",
		"Carolanne",
		"Frank",
		"Heath",
		"Hannah",
		"Siegfried",
		"Arnold",
		"Hedgewick",
		"Emma",
		"Louise",
		"Hugh",
		"Violet",
		"Jed",
		"Englebert",
		"Steve",
		"Crispin"};
}

StartScreen::StartScreen(sf::RenderWindow& renderWindow, SharedData& sharedData)
	:BaseScreen					(renderWindow, sharedData),
	m_currentMenu				(GameSelect),
	m_padding					(10.f),
	m_starField					(sharedData.Textures),
	m_mapSelect					(sharedData.Fonts.Get("assets/fonts/default.ttf")),
	m_mainMenuContainer			(renderWindow),
	m_optionsContainer			(renderWindow),
	m_controlMenuContainerLocal	(renderWindow),
	m_controlMenuContainerTimeTrial(renderWindow),
	m_mapSelectContainer		(renderWindow),
	m_cursorTexture				(sharedData.Textures.Get("assets/textures/ui/cursor.png")),
	m_controlSelectTexture		(sharedData.Textures.Get("assets/textures/ui/ControlSelect.png"))
{
	//title sprites
	m_titleSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/title.png"));
	m_titleSprite.setOrigin(static_cast<float>(m_titleSprite.getTexture()->getSize().x / 2u),
							static_cast<float>(m_titleSprite.getTexture()->getSize().y / 2u));

	m_playerSelectSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/player_select.png"));
	m_playerSelectSprite.setOrigin(static_cast<float>(m_playerSelectSprite.getTexture()->getSize().x / 2u),
									static_cast<float>(m_playerSelectSprite.getTexture()->getSize().y / 2u));

	m_mapSelectSprite.setTexture(sharedData.Textures.Get("assets/textures/ui/map_select.png"));
	m_mapSelectSprite.setOrigin(static_cast<float>(m_mapSelectSprite.getTexture()->getSize().x / 2u),
								/*static_cast<float>(m_mapSelectSprite.getTexture()->getSize().y / 2u)*/0.f);
	

	//build UI components
	m_buttonNext = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), sharedData.Textures.Get("assets/textures/ui/buttons/right_arrow.png"));
	m_buttonPrev = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), sharedData.Textures.Get("assets/textures/ui/buttons/left_arrow.png"));

	m_BuildPlayTypeMenu();
	m_BuildOptionsMenu();
	m_BuildControlMenuLocal();
	m_BuildControlMenuTimeTrial();
	m_BuildRaceTypeMenu();

	//cursor
	m_cursorSprite = AniSprite(m_cursorTexture, 40, 40);
	m_cursorSprite.setFrameRate(12.5f);
	m_cursorSprite.play();

	for(auto& t : m_playlistText)
	{
		t.setCharacterSize(25u);
		t.setFont(sharedData.Fonts.Get("assets/fonts/default.ttf"));
	}

	m_UpdateViewRatio();
}

void StartScreen::m_HandleCustomEvent()
{
	if(m_hasFocus)
	{
		m_mainMenuContainer.HandleEvent(m_event);
		m_optionsContainer.HandleEvent(m_event);
		m_controlMenuContainerLocal.HandleEvent(m_event);
		m_controlMenuContainerTimeTrial.HandleEvent(m_event);
		m_mapSelectContainer.HandleEvent(m_event);
	}

	if(m_event.type == sf::Event::KeyReleased
		&& m_hasFocus)
	{
		switch(m_event.key.code)
		{
		case sf::Keyboard::Escape:
			m_return = QUIT;
			m_running = false;
			break;

		default: break;
		}
	}
}

void StartScreen::m_Update(float dt)
{
	//update background
	m_starField.Update(dt, Helpers::Rectangles::GetViewRect(m_sharedData.Screen.DefaultView));

	//update cursor
	sf::Vector2f mousePos = m_renderWindow.mapPixelToCoords(sf::Mouse::getPosition(m_renderWindow));
	m_cursorSprite.setPosition(mousePos);
	m_cursorSprite.update();
	
	//poll for controller connection
	for(auto& i : m_availableInputs) i = true;
	m_availableInputs[2] = sf::Joystick::isConnected(0);
	m_availableInputs[3] = sf::Joystick::isConnected(1);

	//update the UI
	m_padding = static_cast<float>(m_renderWindow.getSize().x) * 0.015625f; //10px @ 640x480

	m_AlignButtonLeft(m_buttonPrev);
	m_AlignButtonRight(m_buttonNext);

	//perform resizing
	switch (m_currentMenu)
	{
	case GameSelect:
	default:
		{ 
			//update title sprite position
			m_titleSprite.setPosition(m_renderWindow.getView().getCenter());
			//and button positions
			sf::Vector2f size = m_timeTrialButton->GetSize();
			m_timeTrialButton->setPosition(m_renderWindow.getView().getCenter());
			m_timeTrialButton->move(0.f, 134.f); //Hack to line up with title texture
			size.x = 0.f;

			m_localPlayButton->setPosition(m_timeTrialButton->getPosition() + size);
			m_netPlayButton->setPosition(m_localPlayButton->getPosition() + size);
			m_optionsButton->setPosition(m_netPlayButton->getPosition() + size);
			m_quitButton->setPosition(m_optionsButton->getPosition() + size);


			//update position of options menu components
			m_optionsApplyButton->setPosition(m_quitButton->getPosition() + sf::Vector2f(-110.f, 0.f));
			m_optionsCloseButton->setPosition(m_quitButton->getPosition() + sf::Vector2f(110.f, 0.f));
			m_optionsMusicVolume->setPosition(m_optionsButton->getPosition() + sf::Vector2f(-125.f, -25.f));
			m_optionsEffectsVolume->setPosition(m_optionsButton->getPosition() + sf::Vector2f(-125.f, 25.f));
			m_optionsResolution->setPosition(m_localPlayButton->getPosition() + sf::Vector2f(-146.f, -80.f));
			m_optionsMultisampling->setPosition(m_optionsResolution->getPosition() + sf::Vector2f(190.f, 0.f));
			m_optionsVSync->setPosition(m_localPlayButton->getPosition() + sf::Vector2f(-120.f, -10.f));
			m_optionsFullScreen->setPosition(m_localPlayButton->getPosition() + sf::Vector2f(-120.f, 50.f));

			m_optionsContainer.Update(dt);
		}
		break;

	case MultiControlSelect:
		{
			//lay out vehicle and control selection components
			const float uiWidth = m_sharedData.Screen.ViewSize.x * 0.8f;
			const float spacing = uiWidth / 4.f;
			sf::Vector2f position((m_sharedData.Screen.ViewSize.x - uiWidth) / 2.f, 270.f);
			position.x += spacing / 2.f;
			for(auto& i : m_localVehicleSelect)
			{
				i->setPosition(position);
				position.x += spacing;
			}
			position = m_localVehicleSelect[0]->getPosition();
			position.y += 160.f; //TODO magic number
			for(auto& p : m_playerColours)
			{
				p.setPosition(position);
				position.x += spacing;
			}
			//control selections
			position = m_localVehicleSelect[0]->getPosition();
			position.y += 260.f;
			for(auto& c : m_localControlSelect)
			{
				c->setPosition(position);
				position.x += spacing;
				//update control availability
				m_availableInputs[c->SelectedIndex()] = false;
			}
			m_availableInputs.back() = true;

			//text boxes for names
			position = m_localVehicleSelect[0]->getPosition();
			position.y += 380.f;
			for(auto& n: m_localTextbox)
			{
				n->setPosition(position);
				position.x += spacing;
			}

			m_raceTypeSelect->setPosition(m_renderWindow.getView().getCenter());
			m_raceTypeSelect->move(0.f, 220.f);
			//update title sprite position
			m_playerSelectSprite.setPosition(m_renderWindow.getView().getCenter());
			//update animations
			m_controlMenuContainerLocal.Update(dt);

			//update enabled indices for vehicles
			if(m_localVehicleSelect[3]->SelectedIndex() == SelectedVehicle::Disabled)
			{
				m_localControlSelect[3]->SelectIndex(4);
				m_localControlSelect[3]->SetEnabled(false);
				m_localVehicleSelect[2]->SetIndexEnabled(SelectedVehicle::Disabled, true);
			}
			else
			{
				m_localControlSelect[3]->SetEnabled();
				m_localVehicleSelect[2]->SetIndexEnabled(SelectedVehicle::Disabled, false);
			}

			if(m_localVehicleSelect[2]->SelectedIndex() == SelectedVehicle::Disabled)
			{
				m_localControlSelect[2]->SelectIndex(4);
				m_localControlSelect[2]->SetEnabled(false);
				m_localVehicleSelect[3]->SetEnabled(false);
			}
			else
			{
				m_localControlSelect[2]->SetEnabled();
				m_localVehicleSelect[3]->SetEnabled(true);
			}

			//and controls
			for(auto& c : m_localControlSelect)
			{
				for(auto i = 0u; i < m_availableInputs.size(); ++i)
					c->SetIndexEnabled(i, m_availableInputs[i]);
			}
		}
		break;

	case SingleControlSelect:
		m_timeTrialControlSelect->setPosition(m_sharedData.Screen.ViewSize / 2.f);
		for(auto i = 0u; i < m_availableInputs.size(); ++i)
			m_timeTrialControlSelect->SetIndexEnabled(i, m_availableInputs[i]);

		m_timeTrialVehicleSelect->setPosition(m_timeTrialControlSelect->getPosition());
		m_timeTrialVehicleSelect->move(0.f, -m_timeTrialVehicleSelect->GetSize().y);

		m_timeTrialTextBox->setPosition(m_timeTrialControlSelect->getPosition());
		m_timeTrialTextBox->move(0.f, 200.f);
		m_controlMenuContainerTimeTrial.Update(dt); //updates animations

		//update title sprite position
		m_playerSelectSprite.setPosition(m_renderWindow.getView().getCenter());
			
		break;

	case MapSelect:
		{
			const float centre = m_sharedData.Screen.ViewSize.x / 2.f;
			m_mapSelectButtonAdd->setPosition(centre - 430.f, 690.f);
			m_mapSelectLapCount->setPosition(centre - 310.f, 690.f);
			m_mapSelectCheckBox->setPosition(centre + 200.f, 690.f);
			m_mapSelectButtonRemove->setPosition(centre + 400.f, 690.f);
			//update map selection
			m_mapSelect.Update(dt);

			//update playlist string
			sf::FloatRect bounds = m_mapSelect.GetVisibleBounds();
			m_playlistText[0].setPosition(bounds.left + 30.f, bounds.top + bounds.height + 140.f);
			m_playlistText[1].setPosition(m_playlistText[0].getPosition() + sf::Vector2f(300.f, 0.f));
			m_playlistText[2].setPosition(m_playlistText[1].getPosition() + sf::Vector2f(100.f, 0.f));

			//update title sprite
			m_mapSelectSprite.setPosition(m_renderWindow.getView().getCenter().x, m_playlistText[0].getPosition().y - 62.f);
		}
		break;
	}
}

void StartScreen::m_Render()
{	
	//----draw to extract buffer----
	m_extractBuffer.clear(sf::Color::Transparent);
	m_extractBuffer.draw(m_starField);
	m_extractBuffer.draw(m_mapSelect);

	//TODO draw other stuff
	//draw cursor last
	m_extractBuffer.draw(m_cursorSprite);
	m_extractBuffer.display();

	//----scale and extract to blur buffer----
	m_extractSprite.setScale(m_blurmapScale, m_blurmapScale);
	m_verticalBuffer.clear(sf::Color::Transparent);
	m_verticalBuffer.draw(m_extractSprite, &m_sharedData.Screen.Shaders.BloomExtract);
	m_verticalBuffer.display();
	m_extractSprite.setScale(1.f, 1.f);

	m_sharedData.Screen.Shaders.BloomBlur.setParameter("orientation", 0.f);
	m_horizontalBuffer.clear(sf::Color::Transparent);
	m_horizontalBuffer.draw(sf::Sprite(m_verticalBuffer.getTexture()), &m_sharedData.Screen.Shaders.BloomBlur);
	m_horizontalBuffer.display();

	m_sharedData.Screen.Shaders.BloomBlur.setParameter("orientation", 1.f);
	m_verticalBuffer.clear(sf::Color::Transparent);
	m_verticalBuffer.draw(sf::Sprite(m_horizontalBuffer.getTexture()), &m_sharedData.Screen.Shaders.BloomBlur);
	m_verticalBuffer.display();

	//----draw to main window----
	m_renderWindow.clear(sf::Color::Blue);

	m_renderWindow.pushGLStates();

	m_sharedData.Screen.Shaders.BloomBlend.setParameter("baseTexture", m_extractBuffer.getTexture());
	m_sharedData.Screen.Shaders.BloomBlend.setParameter("glowTexture", m_verticalBuffer.getTexture());
	m_renderWindow.draw(m_extractSprite, &m_sharedData.Screen.Shaders.BloomBlend);

	//draw title gfx
	switch(m_currentMenu)
	{
	case GameSelect:
		m_renderWindow.draw(m_titleSprite);
		break;
	case MultiControlSelect:
	case SingleControlSelect:
		m_renderWindow.draw(m_playerSelectSprite);
		if(m_currentMenu == MultiControlSelect)
		{
			sf::RenderStates states(sf::BlendAdd);
			states.shader = &m_playerBlend;
			for(auto i = 0u; i < m_playerColours.size(); ++i)
			{
				m_playerBlend.setParameter("colour", PlayerColours::Light(i));
				m_renderWindow.draw(m_playerColours[i], states);
			}
		}
		break;
	case MapSelect:
		m_renderWindow.draw(m_mapSelectSprite);
		break;
	default: break;
	}

	//draw text on top
	if(m_mapSelect.Visible())
	{
		for(const auto& t : m_playlistText)
			m_renderWindow.draw(t);
		m_renderWindow.draw(m_mapSelect.GetMapname());
	}

	//draw ui on top
	m_renderWindow.draw(m_mainMenuContainer);
	m_renderWindow.draw(m_optionsContainer);
	m_renderWindow.draw(m_controlMenuContainerLocal);
	m_renderWindow.draw(m_controlMenuContainerTimeTrial);
	m_renderWindow.draw(m_mapSelectContainer);
	m_renderWindow.draw(m_cursorSprite/*, sf::BlendAdd*/);
	m_renderWindow.popGLStates();
	m_renderWindow.display();

	m_DrawFps();
}

void StartScreen::m_OnStart()
{
	//load default menu
	m_currentMenu = GameSelect;
	m_renderWindow.setMouseCursorVisible(false);

	m_mainMenuContainer.SetVisible();

	//parse list of maps
	m_ParseMaplist();

	//play music
	m_sharedData.AudioManager.PlayMusic(Audio::Title);

	//feed in some names
	std::random_shuffle(names.begin(), names.end());
	int i = Helpers::Random::Int(0, names.size() - 1);
	m_timeTrialTextBox->SetText(names[i]);

	//make sure names are random, but unique
	for(auto& n : m_localTextbox)
	{
		i = (i + 1) % names.size();
		n->SetText(names[i]);
	}
}

void StartScreen::m_OnFinish()
{
	m_renderWindow.setMouseCursorVisible(true);

	//stop music
	m_sharedData.AudioManager.StopMusic();
}

void StartScreen::m_OnContextCreate()
{

}

void StartScreen::m_ParseMaplist()
{
	//parse maplist.txt from resource folder
	std::ifstream maplist("assets/maps/maplist.txt");
	std::string mapname;
	while(std::getline(maplist, mapname))
	{
		if(!mapname.empty())
			m_sharedData.MapList.push_back(mapname);
	}
	//remove any duplicate entries
	m_sharedData.MapList.remove_if(RemDup());

	if(m_sharedData.MapList.size() == 0)
		std::cout << "Warning: no map names were loaded from maplist.txt" << std::endl;
	else //we have maps we can create the selector for
		m_mapSelect.Create(m_sharedData.MapList, m_sharedData.Textures);

}

///-----------UI Stuffs------------///

void StartScreen::m_BuildPlayTypeMenu()
{
	//create buttons
	m_timeTrialButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), m_sharedData.Textures.Get("assets/textures/ui/buttons/button_timetrial.png"));
	m_timeTrialButton->SetCallback([this]()
	{
		m_HandleTimeTrialClick();
	});
	m_mainMenuContainer.AddComponent(m_timeTrialButton);

	m_localPlayButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), m_sharedData.Textures.Get("assets/textures/ui/buttons/button_localplay.png"));
	m_localPlayButton->SetCallback([this]()
	{
		m_HandleLocalPlayClick();
	});
	m_mainMenuContainer.AddComponent(m_localPlayButton);

	m_netPlayButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), m_sharedData.Textures.Get("assets/textures/ui/buttons/button_netplay.png"));
	m_mainMenuContainer.AddComponent(m_netPlayButton);

	m_optionsButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), m_sharedData.Textures.Get("assets/textures/ui/buttons/button_options.png"));
	m_optionsButton->SetCallback([this]()
	{
		m_HandleOptionsClick();
	});
	m_mainMenuContainer.AddComponent(m_optionsButton);

	m_quitButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(), m_sharedData.Textures.Get("assets/textures/ui/buttons/button_quit.png"));
	m_quitButton->SetCallback([this]()
	{
		m_HandleQuitClick();
	});
	m_mainMenuContainer.AddComponent(m_quitButton);
}

void StartScreen::m_BuildOptionsMenu()
{
	//drop down lists
	m_optionsResolution = std::make_shared<UIComboBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/resolution_dd.png"));

	const float currentRatio = m_GetVideoModeRatio(sf::VideoMode::getDesktopMode());
	const std::vector<sf::VideoMode>& modes = sf::VideoMode::getFullscreenModes();
	for(auto& m : modes)
		if(m.bitsPerPixel == 32 && m.width != 720u && m_GetVideoModeRatio(m) == currentRatio)
			m_optionsResolution->AddItem(m_VideoModeToString(m), (m.width << 16 | m.height)); //TODO set data val to bitshift

	//debug
	m_optionsResolution->AddItem(m_VideoModeToString(sf::VideoMode(800u, 600u)), (800 << 16 | 600));
	m_optionsResolution->AddItem(m_VideoModeToString(sf::VideoMode(1920u, 1080u)), (1920 << 16 | 1080));

	m_optionsMultisampling = std::make_shared<UIComboBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/multisample_dd.png"));
	m_optionsMultisampling->AddItem("Off", 0);
	m_optionsMultisampling->AddItem("1x", 1);
	m_optionsMultisampling->AddItem("2x", 2);
	m_optionsMultisampling->AddItem("4x", 4);
	m_optionsMultisampling->AddItem("8x", 8);


	//check boxes
	m_optionsVSync = std::make_shared<UICheckBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/check.png"));
	m_optionsVSync->SetAlignment(UICheckBox::Alignment::Right);
	m_optionsVSync->SetText("Enable VSync");

	m_optionsFullScreen = std::make_shared<UICheckBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/check.png"));
	m_optionsFullScreen->SetAlignment(UICheckBox::Alignment::Right);
	m_optionsFullScreen->SetText("Full Screen");

	//sliders
	m_optionsMusicVolume = std::make_shared<UISlider>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
														m_sharedData.Textures.Get("assets/textures/ui/slider_handle.png"));
	m_optionsMusicVolume->SetText("Music Volume:");
	m_optionsEffectsVolume = std::make_shared<UISlider>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
														m_sharedData.Textures.Get("assets/textures/ui/slider_handle.png"));
	m_optionsEffectsVolume->SetText("Effects Volume:");	
	
	//buttons
	m_optionsApplyButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(),
														m_sharedData.Textures.Get("assets/textures/ui/buttons/apply.png"));
	m_optionsApplyButton->SetCallback([this]()
	{
		m_HandleApplySettings();		
	});

	m_optionsCloseButton = std::make_shared<UIButton>(m_sharedData.Fonts.Get(),
														m_sharedData.Textures.Get("assets/textures/ui/buttons/close.png"));
	m_optionsCloseButton->SetCallback([this]()
	{
		m_mainMenuContainer.SetVisible();
		m_optionsContainer.SetVisible(false);
	});

	m_optionsContainer.AddComponent(m_optionsCloseButton);
	m_optionsContainer.AddComponent(m_optionsApplyButton);	
	m_optionsContainer.AddComponent(m_optionsEffectsVolume);	
	m_optionsContainer.AddComponent(m_optionsMusicVolume);
	m_optionsContainer.AddComponent(m_optionsFullScreen);
	m_optionsContainer.AddComponent(m_optionsVSync);
	m_optionsContainer.AddComponent(m_optionsMultisampling);
	m_optionsContainer.AddComponent(m_optionsResolution);

	m_optionsContainer.SetVisible(false);

	m_ParseOptions();
}

void StartScreen::m_BuildControlMenuLocal()
{
	//rotate vehicle select by 90 degrees to fit screen better in 4:3
	for(auto& i : m_localVehicleSelect)
	{
		i = std::make_shared<UIMeterSprite>(m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/front_multi.png"),
											m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/back_multi.png"),
											m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/needle_multi.png"), 4u);
		i->SetNeedlePosition(sf::Vector2f(0.f, 70.f));
		i->rotate(-90.f);
		m_controlMenuContainerLocal.AddComponent(i);
	}
	//disable no player for 1 & 2
	m_localVehicleSelect[0]->SetIndexEnabled(SelectedVehicle::Disabled, false);
	m_localVehicleSelect[1]->SetIndexEnabled(SelectedVehicle::Disabled, false);
	m_localVehicleSelect[2]->SelectIndex(SelectedVehicle::Disabled);
	m_localVehicleSelect[3]->SelectIndex(SelectedVehicle::Disabled);

	m_raceTypeSelect = std::make_shared<UIScrollSprite>(m_sharedData.Textures.Get("assets/textures/ui/mode_select/front.png"),
														m_sharedData.Textures.Get("assets/textures/ui/mode_select/back.png"));
	m_raceTypeSelect->SetTextSize(22u);
	m_raceTypeSelect->SetTextOffset(sf::Vector2f(0.f, 69.f));
	m_raceTypeSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/mode_select/item01.png"),
								"Elimination",
								m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
	m_raceTypeSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/mode_select/item02.png"),
								"Split Screen",
								m_sharedData.Fonts.Get("assets/fonts/console.ttf"));

	//control selections
	for(auto& c : m_localControlSelect)
	{
		c = std::make_shared<UIScrollSprite>(m_sharedData.Textures.Get("assets/textures/ui/control_select/front_multi.png"),
																	m_sharedData.Textures.Get("assets/textures/ui/control_select/back_multi.png"));
		c->SetTextOffset(sf::Vector2f(0.f, 36.f));
		c->SetTextSize(22u);

		c->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item4_multi.png"),
											"Keyset 1",
											m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
		c->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item3_multi.png"),
											"Keyset 2",
											m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
		c->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item2_multi.png"),
											"Controller 1",
											m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
		c->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item1_multi.png"),
											"Controller 2",
											m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
		c->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item0_multi.png"),
											"Computer",
											m_sharedData.Fonts.Get("assets/fonts/console.ttf"));

		c->SelectIndex(4u);
		m_controlMenuContainerLocal.AddComponent(c);
	}

	//text boxes for names
	for(auto& n : m_localTextbox)
	{
		n = std::make_shared<UITextBox>(m_sharedData.Fonts.Get("assets/fonts/biro.ttf"), sf::Color::White, sf::Color::Blue);
		n->SetTexture(m_sharedData.Textures.Get("assets/textures/ui/textBox.png"));
		n->ShowBorder(false);
		const float scale = 0.8f;
		n->setScale(scale, scale);
		m_controlMenuContainerLocal.AddComponent(n);
	}

	//the order in which components are added to container are important when navigating menus
	m_controlMenuContainerLocal.AddComponent(m_raceTypeSelect);
	m_controlMenuContainerLocal.AddComponent(m_buttonNext);
	m_controlMenuContainerLocal.AddComponent(m_buttonPrev);
	m_controlMenuContainerLocal.SetVisible(false);

	for(auto& p : m_playerColours)
	{
		p.setTexture(m_sharedData.Textures.Get("assets/textures/ui/play_bar.png"), true);
		Helpers::Position::CentreOrigin(p);
	}
	m_playerBlend.loadFromMemory(blendOverlay, sf::Shader::Fragment);
}

void StartScreen::m_BuildControlMenuTimeTrial()
{
	//control selection
	m_timeTrialControlSelect = std::make_shared<UIScrollSprite>(m_sharedData.Textures.Get("assets/textures/ui/control_select/front.png"),
																m_sharedData.Textures.Get("assets/textures/ui/control_select/back.png"));
	m_timeTrialControlSelect->SetItemOffset(sf::Vector2f(-60.f, 0.f));
	m_timeTrialControlSelect->SetTextOffset(sf::Vector2f(0.f, 70.f));

	m_timeTrialControlSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item4.png"),
										"Keyset One",
										m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
	m_timeTrialControlSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item3.png"),
										"Keyset Two",
										m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
	m_timeTrialControlSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item2.png"),
										"Controller One",
										m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
	m_timeTrialControlSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item1.png"),
										"Controller Two",
										m_sharedData.Fonts.Get("assets/fonts/console.ttf"));
	m_timeTrialControlSelect->AddItem(m_sharedData.Textures.Get("assets/textures/ui/control_select/item0.png"),
										"Computer",
										m_sharedData.Fonts.Get("assets/fonts/console.ttf"));

	//vehicle selection
	m_timeTrialVehicleSelect = std::make_shared<UIMeterSprite>(m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/front.png"),
																m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/back.png"),
																m_sharedData.Textures.Get("assets/textures/ui/vehicle_select/needle.png"));
	m_timeTrialVehicleSelect->SetNeedlePosition(sf::Vector2f(-4.f, 90.f));

	//name input
	m_timeTrialTextBox = std::make_shared<UITextBox>(m_sharedData.Fonts.Get("assets/fonts/biro.ttf"), sf::Color::White, sf::Color::Blue);
	m_timeTrialTextBox->SetTexture(m_sharedData.Textures.Get("assets/textures/ui/textBox.png"));
	m_timeTrialTextBox->ShowBorder(false);

	//adding components in correct order is important when navigating on screen
	m_controlMenuContainerTimeTrial.AddComponent(m_timeTrialVehicleSelect);
	m_controlMenuContainerTimeTrial.AddComponent(m_timeTrialControlSelect);
	m_controlMenuContainerTimeTrial.AddComponent(m_timeTrialTextBox);
	//buttons
	m_controlMenuContainerTimeTrial.AddComponent(m_buttonNext);
	m_controlMenuContainerTimeTrial.AddComponent(m_buttonPrev);

	m_controlMenuContainerTimeTrial.SetVisible(false);	
}

void StartScreen::m_BuildRaceTypeMenu()
{
	//--new menu--//
	m_mapSelectLapCount = std::make_shared<UIComboBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/lapcount_base.png"));
	m_mapSelectLapCount->AddItem("2", 2);
	m_mapSelectLapCount->AddItem("3", 3);
	m_mapSelectLapCount->AddItem("4", 4);
	m_mapSelectLapCount->AddItem("6", 6);
	m_mapSelectLapCount->AddItem("8", 8);
	m_mapSelectLapCount->AddItem("12", 12);

	m_mapSelectCheckBox = std::make_shared<UICheckBox>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/check.png"));
	m_mapSelectCheckBox->SetText("Reverse");
	m_mapSelectCheckBox->SetAlignment(UICheckBox::Alignment::Right);

	m_mapWrap = std::make_shared<UIMapSelectWrapper>(m_mapSelect);

	m_mapSelectButtonAdd = std::make_shared<UIButton>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/buttons/map_add.png"));
	m_mapSelectButtonAdd->SetText("Add");
	m_mapSelectButtonAdd->SetCallback([this]()
	{
		m_HandleRaceTypeAddClick();
	});
	m_mapSelectButtonRemove = std::make_shared<UIButton>(m_sharedData.Fonts.Get("assets/fonts/console.ttf"),
												m_sharedData.Textures.Get("assets/textures/ui/buttons/map_add.png"));
	m_mapSelectButtonRemove->SetText("Remove");
	m_mapSelectButtonRemove->SetCallback([this]()
	{
		m_HandleRaceTypeRemoveClick();
	});

	//order in which components are added is important
	m_mapSelectContainer.AddComponent(m_mapWrap);
	m_mapSelectContainer.AddComponent(m_mapSelectButtonAdd);
	m_mapSelectContainer.AddComponent(m_mapSelectLapCount);
	m_mapSelectContainer.AddComponent(m_mapSelectCheckBox);
	m_mapSelectContainer.AddComponent(m_mapSelectButtonRemove);
	m_mapSelectContainer.AddComponent(m_buttonNext);
	m_mapSelectContainer.AddComponent(m_buttonPrev);
	m_mapSelectContainer.SetVisible(false);
}

void StartScreen::m_UpdatePlaylistString()
{
	//reset old vals
	for(auto& t : m_playlistText)
		t.setString("");

	for(const auto& s : m_sharedData.PlayList)
	{
		sf::String str = m_playlistText[0].getString();
		str += s.MapName + "\n";
		m_playlistText[0].setString(str);

		str = m_playlistText[1].getString();
		str += s.Laps + "\n";
		m_playlistText[1].setString(str);

		str = m_playlistText[2].getString();
		if(s.Reverse) str += "Reverse";
		str += "\n";
		m_playlistText[2].setString(str);
		
		//m_playlistString += "\t" + s.MapName + "\t\t\t\t\t\tLaps: " + s.Laps;
		//if (s.Reverse) m_playlistString += "\t\t\t\tReverse";
		//m_playlistString += "\n";
	}
}

void StartScreen::m_AlignButtonRight(UIButton::Ptr& button)
{
	sf::Vector2f offset = button->GetSize() / 2.f;
	offset.x += m_padding;
	offset.y += m_padding;
	button->setPosition(m_sharedData.Screen.ViewSize - offset);
}

void StartScreen::m_AlignButtonLeft(UIButton::Ptr& button)
{
	sf::Vector2f offset = button->GetSize() / 2.f;
	offset.x += m_padding;
	offset.y = m_sharedData.Screen.ViewSize.y - (m_padding + offset.y);
	button->setPosition(offset);
}

//----functions for applying / modifying options----//
void StartScreen::m_ParseOptions()
{
	VideoSettings settings("config.cfg");
	m_optionsResolution->SelectItem(m_VideoModeToString(settings.getResolution()));
	m_optionsMultisampling->SelectItem(settings.getMultiSamples());
	m_optionsVSync->Check(settings.getVsync());
	m_optionsFullScreen->Check(settings.getFullcreen());
	m_optionsMusicVolume->SetValue(settings.getMusicVolume());
	m_optionsEffectsVolume->SetValue(settings.getSfxVolume());
}

void StartScreen::m_UpdateViewRatio()
{
	float ratio = (static_cast<float>(m_renderWindow.getSize().x) / static_cast<float>(m_renderWindow.getSize().y));
	m_sharedData.Screen.ViewSize.x = (ratio < 1.4f) ? 1440.f : 1920.f;
	m_sharedData.Screen.ViewSize.y = (ratio > 1.65f || ratio < 1.4f) ? 1080.f : 1200.f;
	m_sharedData.Screen.DefaultView.setSize(m_sharedData.Screen.ViewSize);
	m_sharedData.Screen.DefaultView.setCenter(m_sharedData.Screen.ViewSize / 2.f);
	m_sharedData.Screen.WindowScale = static_cast<float>(m_renderWindow.getSize().x) / m_sharedData.Screen.ViewSize.x;
}

std::string StartScreen::m_VideoModeToString(const sf::VideoMode& mode)
{
	std::stringstream stream;
	stream << mode.width << " x " << mode.height;
	return stream.str();
}

float StartScreen::m_GetVideoModeRatio(const sf::VideoMode& mode)
{
	return static_cast<float>(mode.width) / mode.height;
}