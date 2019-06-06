#include <Game/StartScreen.h>
#include <Game/GameScreen.h>
#include <Game/Shaders/Bloom.h>
#include <Game/Shaders/GaussianBlur.h>
#include <Game/SaveSettings.h>
#include <fstream>

#include <Game/LoadingDisplay.h>


int main()
{	
	//init 3d renderer
	glewInit();

	//load settings - TODO make exception safe
	VideoSettings settings("config.cfg");
	sf::ContextSettings contextSettings;
	contextSettings.antialiasingLevel = settings.getMultiSamples();

	if(!settings.getResolution().isValid())
	{
		std::cerr << "Invalid video mode, try deleting config file and restarting the application." << std::endl;
		return 1;
	}
	//TODO make this exception safe
	//we can't just swallow any exceptions here as it contains the entire program

	//create window from settings
	sf::RenderWindow renderWindow;
	renderWindow.create(settings.getResolution(), "SFML Window", settings.getFullcreen() ? sf::Style::Fullscreen : sf::Style::Close, contextSettings);
	renderWindow.setVerticalSyncEnabled(settings.getVsync());
	sf::Vector2f v(1440.f, 1080.f); ///bblleeeaargh, we don't wants this
	sf::View view(v / 2.f, v);
	renderWindow.setView(view);
	renderWindow.setActive(false);

	//create a shared data object for sharing information between screens
	Game::SharedData sharedData;
	sharedData.AudioManager.SetGlobalEffectsVolume(settings.getSfxVolume());
	sharedData.AudioManager.SetGlobalMusicVolume(settings.getMusicVolume());

	//load shaders
	if(!sf::Shader::isAvailable())
	{
		sf::err() << "Shader hardware unavailable. This game requires at least OpenGL 1.2 hardware acceleration." << std::endl;
		return 1;
	}
	else
	{
		if(!sharedData.Screen.Shaders.BloomExtract.loadFromMemory(bloomExtract, sf::Shader::Fragment)
			|| !sharedData.Screen.Shaders.BloomBlur.loadFromMemory(gaussianBlur, sf::Shader::Fragment)
			|| !sharedData.Screen.Shaders.BloomBlend.loadFromMemory(bloomBlend, sf::Shader::Fragment))
		{
			sf::err() << "Unable to create one or more shaders" << std::endl;
		}
	}


	//create a loading thread
	LoadingDisplay ld(renderWindow);
	sf::Thread thread(&LoadingDisplay::ThreadFunc, &ld);
	thread.launch();

	//create game screens
	std::vector< std::unique_ptr<Game::BaseScreen> > screens;
	int screen = 0;

	screens.push_back(std::unique_ptr<Game::BaseScreen>(new Game::StartScreen(renderWindow, sharedData)));
	screens.push_back(std::unique_ptr<Game::BaseScreen>(new Game::GameScreen(renderWindow, sharedData)));

	//kill thread and enter main loop
	ld.Running = false;
	thread.wait();

	while(screen >= 0)
	{
		screen = screens[screen]->Run();
	}

	return 0;
}