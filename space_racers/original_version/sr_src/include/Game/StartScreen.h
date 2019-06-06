///header file for start screen class///
#ifndef START_SCREEN_H_
#define START_SCREEN_H_

#include <Game/BaseScreen.h>
#include <Game/StarField.h>
#include <Game/MapSelect.h>
#include <Game/UIContainer.h>
#include <Game/UIButton.h>
#include <Game/UIScrollSprite.h>
#include <Game/UIMeterSprite.h>
#include <Game/UITextBox.h>
#include <Game/UICheckBox.h>
#include <Game/UIComboBox.h>
#include <Game/UIMapSelectWrapper.h>
#include <Game/UISlider.h>

#include <set>

namespace Game
{	
	//default screen for main menu
	class StartScreen final : public BaseScreen
	{
	public:
		StartScreen(sf::RenderWindow& renderWindow, SharedData& sharedData);

	private:
		enum CurrentMenu
		{
			GameSelect,
			MultiControlSelect,
			SingleControlSelect,
			MapSelect
		} m_currentMenu;


		//functor for removing dup track names
		struct RemDup
		{
			std::set<std::string> foundElements;

			bool operator()(std::string& a)
			{
				if(foundElements.find(a) != foundElements.end())
				{
					return true;
				}
				else
				{
					foundElements.insert(a);
					return false;
				};
			};
		};

		StarField m_starField;// for prettiness and woo
		const sf::Texture& m_cursorTexture;
		AniSprite m_cursorSprite;
		sf::Sprite m_titleSprite, m_playerSelectSprite, m_mapSelectSprite;
		void m_HandleCustomEvent();
		void m_Update(float dt);
		void m_Render();

		void m_OnStart();
		void m_OnFinish();
		void m_OnContextCreate();

		void m_ParseMaplist();


		//---UI stuffs----//
		enum SelectedVehicle
		{
			Ship = 0u,
			Car = 1u,
			Bike = 2u,
			Disabled = 3u
		};
		//---main menu---//
		UIContainer m_mainMenuContainer;
		UIButton::Ptr m_timeTrialButton; //we need to keep member refs so positions can be updated
		UIButton::Ptr m_localPlayButton;
		UIButton::Ptr m_netPlayButton;
		UIButton::Ptr m_optionsButton;
		UIButton::Ptr m_quitButton;
		void m_BuildPlayTypeMenu();
		void m_HandleLocalPlayClick();
		void m_HandleOptionsClick();
		void m_HandleQuitClick();
		void m_HandleTimeTrialClick();

		UIButton::Ptr m_buttonNext;
		UIButton::Ptr m_buttonPrev;

		//----options menu----//
		UIContainer m_optionsContainer;
		UIButton::Ptr m_optionsApplyButton;
		UIButton::Ptr m_optionsCloseButton;
		UICheckBox::Ptr m_optionsFullScreen;
		UICheckBox::Ptr m_optionsVSync;
		UIComboBox::Ptr m_optionsResolution;
		UIComboBox::Ptr m_optionsMultisampling;
		UISlider::Ptr m_optionsMusicVolume;
		UISlider::Ptr m_optionsEffectsVolume;
		void m_BuildOptionsMenu();
		void m_ParseOptions();
		void m_HandleApplySettings();
		void m_UpdateViewRatio();

		std::string m_VideoModeToString(const sf::VideoMode& mode);
		float m_GetVideoModeRatio(const sf::VideoMode& mode);

		//---local control menu---//
		UIContainer m_controlMenuContainerLocal;
		std::array<UIScrollSprite::Ptr, 4u> m_localControlSelect;
		std::array<UIMeterSprite::Ptr, 4u> m_localVehicleSelect;
		std::array<UITextBox::Ptr, 4u> m_localTextbox;
		UIScrollSprite::Ptr m_raceTypeSelect;
		void m_BuildControlMenuLocal();
		void m_HandleControlLocalNextClick();
		void m_HandleControlLocalPrevClick();
		std::array<sf::Sprite, 4u> m_playerColours;
		sf::Shader m_playerBlend;
		const sf::Texture& m_controlSelectTexture;

		//---time trial control menu---//
		UIContainer m_controlMenuContainerTimeTrial;
		UIScrollSprite::Ptr m_timeTrialControlSelect;
		UIMeterSprite::Ptr m_timeTrialVehicleSelect;
		UITextBox::Ptr m_timeTrialTextBox;
		void m_BuildControlMenuTimeTrial();
		void m_HandleControlTimeTrialNextClick();
		void m_HandleControlTimeTrialPrevClick();

		//---map select menu---//
		MapSelection m_mapSelect;
		std::array<sf::Text, 3u> m_playlistText;

		UIContainer m_mapSelectContainer;
		UICheckBox::Ptr m_mapSelectCheckBox;
		UIButton::Ptr m_mapSelectButtonAdd;
		UIButton::Ptr m_mapSelectButtonRemove;
		UIComboBox::Ptr m_mapSelectLapCount;
		UIMapSelectWrapper::Ptr m_mapWrap;

		void m_BuildRaceTypeMenu();
		void m_UpdatePlaylistString();
		void m_HandleRaceTypeNextClick();
		void m_HandleRaceTypePrevClick();
		void m_HandleRaceTypeAddClick();
		void m_HandleRaceTypeRemoveClick();


		//aligns prev/next buttons
		float m_padding; //padding for fixed position items
		void m_AlignButtonLeft(UIButton::Ptr& button);
		void m_AlignButtonRight(UIButton::Ptr& button);

		///-----------------------///
		// TODO network play menus //
		///-----------------------///

		//allows UI to check for available controls on player select screen
		std::array<bool, 5> m_availableInputs;

	};
};
#endif