/*Screen classes. Each 'screen' represents a different operating mode of the game, called upon
by the main function. BaseScreen defines the basis for each screen class which provides game loop
timing functions, and screen rendering functions, called when each screen is 'run'. All derived
screen classes must provide their own m_Update() and m_Render() functions as BaseScreen is an ABC.
Run() can also be overridden by derived classes to provide for custom construction and destruction
of objects when screens are switched, although it is recommended to always call BaseScreen::Run()
in each overridden instance. Better explaination of how this works can be found here:
https://github.com/LaurentGomila/SFML/wiki/Tutorial%3A-Manage-different-Screens */
#ifndef _SCREENS_H_
#define _SCREENS_H_

#include <Game/SharedData.h>
#include <Game/StarField.h>

#include <Mesh/MeshLib.h>

//use boost atomic if using VS10
#ifdef _MSC_VER
#if (_MSC_VER == 1600)
#include <boost/atomic.hpp>
#else //prefer C++11 atomic
#include <atomic>
#endif //ver == 1600
#endif //_MSC_VER


//menu IDs - set the return value of a screen object to one of these for the main function to switch
//to the correct screen
const int QUIT = -1;
const int MAIN_MENU = 0;
//const int OPTION_MENU = 1;
const int GAME_SCREEN = 1;

namespace Game
{	
	class BaseScreen : private sf::NonCopyable
	{
	public:
		BaseScreen(sf::RenderWindow& renderWindow, SharedData& sharedData);
		//entry point for all screen classes, called from the main function
		virtual ~BaseScreen(){};
		virtual int Run();

	protected:
		sf::RenderWindow& m_renderWindow;
		sf::Clock m_fpsClock, m_dtClock;

		//contains global data shared between screens
		SharedData& m_sharedData;

		float m_timeScale; //used to speed up and slow down rate of game
		sf::Text m_text;
		sf::Event m_event;
		bool m_running, m_hasFocus;
		int m_return;

		//post process members//
		sf::RenderTexture m_extractBuffer; //render items to glow to this
		sf::Sprite m_extractSprite; //use this to scale down for blur buffer
		sf::RenderTexture m_verticalBuffer, m_horizontalBuffer; //for blur passes
		const float m_blurmapScale;
		//--------------------//

		//override this in derived classes with calls to object logic updates.
		virtual void m_Update(float dt) = 0;
		//override with in any derived classes with object draw calls. Called as often as the system supports after
		//performing any logic updates
		virtual void m_Render() = 0;
		//utility function for drawing the window title. To use it call it from the class's m_Render function.
		float m_DrawFps();
		//utilty function for saving a time stamped screenshot to the same directory as the game executable
		void m_SaveScreenShot();
		//override this in any screen which needs custom event handling
		virtual void m_HandleCustomEvent(){};
		//reads current config settigns and updates options menu
		void m_RefreshOptions();
		//called when screen starts running
		virtual void m_OnStart() = 0;
		//called when screen stops running
		virtual void m_OnFinish() = 0;
		//called when recreating the window / context
		virtual void m_OnContextCreate() = 0;
		void m_LaunchLoadingWindow(const std::string& message = "Loading");
		void m_StopLoadingWindow();
		void m_CreateLoadingDisplay(); //need to call this from thread to properly display sprites :S
		std::string m_loadingString;
		//creates render buffers for post process effect
		void m_CreatePostBuffers();
	private:

		//loading window//
		sf::Text m_loadingMessage;
		sf::Texture m_loadingTextureDiffuse;
		sf::Texture& m_loadingTextureNormal;
		const sf::Texture& m_loadingTextureGlobeNormal;
		sf::Shader m_loadingShader;
		sf::Vector2f m_loadingPosition;
		sf::Sprite m_loadingSprite;
		void m_UpdateLoadingWindow();
		std::atomic<bool> m_threadRunning;
		sf::Thread m_loadingThread;

		static std::array< std::string, 20> m_randomMessages;	
	};


};


#endif