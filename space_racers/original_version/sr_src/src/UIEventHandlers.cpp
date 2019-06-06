//definition for event handlers for GUI events///

#include <Game/StartScreen.h>
#include <Game/SaveSettings.h>

using namespace Game;

//----Start Screen----//
void StartScreen::m_HandleLocalPlayClick()
{
	m_currentMenu = MultiControlSelect;
	m_sharedData.Game.Rules = GameRules::Elimination;

	//hide main menu
	m_mainMenuContainer.SetVisible(false);
	//show selectd menu
	m_controlMenuContainerLocal.SetVisible();

	m_buttonNext->SetCallback([this]()
	{
		m_HandleControlLocalNextClick();
	});
	m_buttonPrev->SetCallback([this]()
	{
		m_HandleControlLocalPrevClick();
	});
}

void StartScreen::m_HandleTimeTrialClick()
{
	m_currentMenu = SingleControlSelect;
	m_sharedData.Game.Rules = GameRules::TimeTrial;

	//hide main menu
	m_mainMenuContainer.SetVisible(false);
	//and show new menu
	m_controlMenuContainerTimeTrial.SetVisible();
	m_buttonNext->SetCallback([this]()
	{
		m_HandleControlTimeTrialNextClick();
	});
	m_buttonPrev->SetCallback([this]()
	{
		m_HandleControlTimeTrialPrevClick();
	});
}

void StartScreen::m_HandleOptionsClick()
{
	m_mainMenuContainer.SetVisible(false);
	m_optionsContainer.SetVisible();
}

void StartScreen::m_HandleQuitClick()
{
	m_return = QUIT;
	m_running = false;
}

void StartScreen::m_HandleControlLocalNextClick()
{
	m_currentMenu = MapSelect;
	m_sharedData.Game.Players.clear();

	//create players for enabled selections
	for(auto i = 0u; i < m_localVehicleSelect.size(); ++i)
	{
		if(m_localVehicleSelect[i]->SelectedIndex() != SelectedVehicle::Disabled)
		{
			m_sharedData.Game.Players.push_back(std::make_shared<PlayerData>(m_localTextbox[i]->GetText(),
												static_cast<ControlSet>(m_localControlSelect[i]->SelectedIndex()),
												static_cast<Vehicle::VehicleType>(m_localVehicleSelect[i]->SelectedIndex()), i));
		}
	}

	//set rules depending on player count
	if(m_sharedData.Game.Players.size() < 3)
		m_sharedData.Game.RoundsToWin = 8u;
	else
		m_sharedData.Game.RoundsToWin = 4u;

	//and combo selection
	if(m_raceTypeSelect->SelectedIndex() == 0)
		m_sharedData.Game.Rules = GameRules::Elimination;
	else
		m_sharedData.Game.Rules = GameRules::SplitScreen;

	//show map select
	m_mapSelect.SetVisible();
	m_mapSelect.SetCentre(sf::Vector2f(m_sharedData.Screen.DefaultView.getCenter().x,
		m_sharedData.Screen.DefaultView.getCenter().y - 40.f));

	//hide menu and show next
	m_controlMenuContainerLocal.SetVisible(false);
	m_mapSelectContainer.SetVisible();
	m_buttonNext->SetCallback([this]()
	{
		m_HandleRaceTypeNextClick();
	});
	m_buttonPrev->SetCallback([this]()
	{
		m_HandleRaceTypePrevClick();
	});
}

void StartScreen::m_HandleControlLocalPrevClick()
{
	m_currentMenu = GameSelect;

	//show main menu again
	m_mainMenuContainer.SetVisible();
	m_controlMenuContainerLocal.SetVisible(false);
}

void StartScreen::m_HandleControlTimeTrialNextClick()
{
	m_currentMenu = MapSelect;
	m_sharedData.Game.Players.clear();

	//apply player data
	m_sharedData.Game.Players.push_back(
		std::make_shared<PlayerData>(m_timeTrialTextBox->GetText(),
									static_cast<ControlSet>(m_timeTrialControlSelect->SelectedIndex()),
									static_cast<Vehicle::VehicleType>(m_timeTrialVehicleSelect->SelectedIndex()), 0u,
									static_cast<Intelligence>(Helpers::Random::Int(0, 3))
									));

	//show map select
	m_mapSelect.SetVisible();
	m_mapSelect.SetCentre(sf::Vector2f(m_sharedData.Screen.DefaultView.getCenter().x,
		m_sharedData.Screen.DefaultView.getCenter().y - 40.f));


	//hide menu and show next
	m_controlMenuContainerTimeTrial.SetVisible(false);
	m_mapSelectContainer.SetVisible();
	m_buttonNext->SetCallback([this]()
	{
		m_HandleRaceTypeNextClick();
	});
	m_buttonPrev->SetCallback([this]()
	{
		m_HandleRaceTypePrevClick();
	});
}

void StartScreen::m_HandleControlTimeTrialPrevClick()
{
	m_currentMenu = GameSelect;

	//show main menu
	m_mainMenuContainer.SetVisible();
	m_controlMenuContainerTimeTrial.SetVisible(false);
}

void StartScreen::m_HandleRaceTypeNextClick()
{	
	//don't do anything without a playlist
	if(m_sharedData.PlayList.empty()) return;
	
	//hide map select
	m_mapSelect.SetVisible(false);
	
	m_return = GAME_SCREEN;
	m_running = false;

	for(auto& t : m_playlistText) t.setString("");

	m_sharedData.Game.Reset(); //clear out scores from last tournament

	m_mapSelectContainer.SetVisible(false);
}

void StartScreen::m_HandleRaceTypePrevClick()
{
	//hide map select
	m_mapSelect.SetVisible(false);	
	m_mapSelectContainer.SetVisible(false);

	//show control select based on mode
	if(m_sharedData.Game.Rules != GameRules::TimeTrial)
	{	
		m_currentMenu = MultiControlSelect;

		m_controlMenuContainerLocal.SetVisible();
		m_buttonNext->SetCallback([this]()
		{
			m_HandleControlLocalNextClick();
		});
		m_buttonPrev->SetCallback([this]()
		{
			m_HandleControlLocalPrevClick();
		});
	}
	else
	{		
		m_currentMenu = SingleControlSelect;

		m_controlMenuContainerTimeTrial.SetVisible();
		m_buttonNext->SetCallback([this]()
		{
			m_HandleControlTimeTrialNextClick();
		});
		m_buttonPrev->SetCallback([this]()
		{
			m_HandleControlTimeTrialPrevClick();
		});
	}
}

void StartScreen::m_HandleRaceTypeAddClick()
{
	if(!m_mapSelect.Animating() && m_sharedData.PlayList.size() < 6u)
	{
		m_sharedData.PlayList.push_back(
			SharedData::RaceInfo(m_mapSelect.GetSelectedValue(),
			m_mapSelectLapCount->GetSelectedText(),
			m_mapSelectCheckBox->Checked())
			);
		m_UpdatePlaylistString();
	}
}

void StartScreen::m_HandleRaceTypeRemoveClick()
{
	if(!m_sharedData.PlayList.empty())
	{
		m_sharedData.PlayList.pop_back();
		m_UpdatePlaylistString();
	}
}

void StartScreen::m_HandleApplySettings()
{
	VideoSettings settings("config.cfg");
	//recreate window if video settings have changed
	if ((m_optionsResolution->GetSelectedText() != m_VideoModeToString(settings.getResolution()))
		|| (m_optionsMultisampling->GetSelectedValue() != settings.getMultiSamples())
		|| (m_optionsFullScreen->Checked() != settings.getFullcreen()))
	{
		//store current settings as fall back
		sf::ContextSettings currentContext, newContext;
		currentContext.antialiasingLevel = settings.getMultiSamples();
		newContext.antialiasingLevel = m_optionsMultisampling->GetSelectedValue();

		sf::VideoMode currentMode, newMode;
		currentMode = settings.getResolution();
		const std::vector<sf::VideoMode>& modes = sf::VideoMode::getFullscreenModes();
		for (auto& m : modes)
		{
			if (m_VideoModeToString(m) == m_optionsResolution->GetSelectedText())
			{
				newMode = m;
				break;
			}
		}

		//attempt to apply new settings
		try
		{
			m_renderWindow.create(newMode, "", m_optionsFullScreen->Checked() ? sf::Style::Fullscreen : sf::Style::Close, newContext);
			m_UpdateViewRatio();
			m_renderWindow.setView(m_sharedData.Screen.DefaultView);
			m_CreatePostBuffers();

			settings.setResolution(newMode);
			settings.setFullScreen(m_optionsFullScreen->Checked());
			settings.setMultiSamples(m_optionsMultisampling->GetSelectedValue());
		}
		catch (...)
		{
			m_renderWindow.create(currentMode, "", settings.getFullcreen() ? sf::Style::Fullscreen : sf::Style::Close, currentContext);
		}
	}
	//apply vsync
	m_renderWindow.setVerticalSyncEnabled(m_optionsVSync->Checked());
	settings.setVsync(m_optionsVSync->Checked());

	m_renderWindow.setMouseCursorVisible(false);

	//apply audio settings
	m_sharedData.AudioManager.SetGlobalMusicVolume(m_optionsMusicVolume->GetValue());
	settings.setMusicVolume(m_optionsMusicVolume->GetValue());

	m_sharedData.AudioManager.SetGlobalEffectsVolume(m_optionsEffectsVolume->GetValue());
	settings.setSfxVolume(m_optionsEffectsVolume->GetValue());
}

//----Game Screen----//