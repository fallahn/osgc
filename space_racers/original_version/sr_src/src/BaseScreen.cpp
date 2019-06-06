#include <Game/BaseScreen.h>
#include <Game/Shaders/Planet.h>
#include <algorithm>

using namespace Game;

BaseScreen::BaseScreen(sf::RenderWindow& renderWindow, SharedData& sharedData)
	: m_timeScale			(1.f),
	m_renderWindow			(renderWindow),
	m_sharedData			(sharedData),
	m_hasFocus				(true),
	m_blurmapScale			(0.25f),
	m_threadRunning			(false),
	m_loadingThread			(&BaseScreen::m_UpdateLoadingWindow, this),
	m_loadingTextureNormal	(sharedData.Textures.Get("assets/textures/ui/loading_normal.png")),
	m_loadingTextureGlobeNormal(sharedData.Textures.Get("assets/textures/environment/globe_normal.png"))
	/*,
	m_loadingMessage		("", m_sharedData.Fonts.Get("assets/fonts/default.ttf"), 60u)*/
{
	//load assets for loading window
	m_CreateLoadingDisplay();

	m_loadingTextureNormal.setRepeated(true);
	m_loadingShader.loadFromMemory(planetMap, sf::Shader::Fragment);
	m_loadingShader.setParameter("globeNormalTexture", m_loadingTextureGlobeNormal);
	m_loadingShader.setParameter("normalTexture", m_loadingTextureNormal);
}

//***public functions***//
int BaseScreen::Run()
{
	m_renderWindow.setView(m_sharedData.Screen.DefaultView);

	//place loading screen here so any screen which
	//loads assets in OnStart will display loading screen
	m_LaunchLoadingWindow(m_randomMessages[Helpers::Random::Int(0, m_randomMessages.size() - 1)]);
	m_CreatePostBuffers(); //to make sure any resizing is properly done across screens
	m_OnStart(); //must be AFTER creating buffers
	m_StopLoadingWindow();

	m_running = true;
	const float fixedStep = 1.f / 30.f;

	while(m_running)
	{
		while(m_renderWindow.pollEvent(m_event))
		{
			//check if window has focus
			switch(m_event.type)
			{
			case sf::Event::GainedFocus:
				m_hasFocus = true;
				//Console::lout << "gained focus" << std::endl;
				break;
			case sf::Event::LostFocus:
				m_hasFocus = false;
				//Console::lout << "lost focus" << std::endl;
				break;
			case sf::Event::Closed:
				return QUIT;
			case sf::Event::KeyPressed:
				if(!m_hasFocus) break;
				switch(m_event.key.code)
				{
				case sf::Keyboard::F5:
					m_SaveScreenShot();
					break;
				case sf::Keyboard::Tab:
					
					break;
				default: break;
				}
			default: break;
			}

			//execute any custom events in derived screens
			m_HandleCustomEvent();
		}

		//perform logic updates
		float frameTime = m_dtClock.restart().asSeconds() * m_timeScale;
		sf::Uint8 maxSteps = 60; //get out clause to prevent "spiral of death"
		while(frameTime > 0.f && maxSteps > 0)
		{
			float dt = std::min(frameTime, fixedStep);
			m_Update(dt);
			frameTime -= dt;
			maxSteps--;
		}

		//draw everything
		m_Render();
	}
	m_OnFinish();

	return m_return;
}

//***protected functions***//
float BaseScreen::m_DrawFps()
{
	float frameRate = 1.f / m_fpsClock.getElapsedTime().asSeconds();
	m_fpsClock.restart();

	m_renderWindow.setTitle("FPS: " + std::to_string(frameRate));
	return frameRate;
}

void BaseScreen::m_SaveScreenShot()
{
		std::time_t time = std::time(NULL);
		struct tm* timeInfo;

		timeInfo = std::localtime(&time);

		char buffer[40];
		std::string fileName;

		strftime(buffer, 40, "screenshot%d_%m_%y_%H_%M_%S.png", timeInfo);

		fileName.assign(buffer);

		sf::Image screenCap = m_renderWindow.capture();
		if(screenCap.saveToFile(fileName)){}
			//Console::lout << "Saved " << fileName << " to disk." << std::endl;

}

void BaseScreen::m_CreatePostBuffers()
{
	m_extractBuffer.create(static_cast<unsigned>(m_sharedData.Screen.ViewSize.x), static_cast<unsigned>(m_sharedData.Screen.ViewSize.y));
	m_extractBuffer.setSmooth(true);

	m_verticalBuffer.create(static_cast<unsigned>(m_sharedData.Screen.ViewSize.x * m_blurmapScale),
							static_cast<unsigned>(m_sharedData.Screen.ViewSize.y * m_blurmapScale));
	m_verticalBuffer.setSmooth(true);

	m_horizontalBuffer.create(m_verticalBuffer.getSize().x, m_verticalBuffer.getSize().y);
	//m_horizontalBuffer.setSmooth(true);

	m_extractSprite.setTexture(m_extractBuffer.getTexture());
	//must resize sprites texture rect if updating size of rendertexture
	m_extractSprite.setTextureRect(sf::IntRect(0, 0, m_extractBuffer.getSize().x, m_extractBuffer.getSize().y));

	//update shader parameters with new settings
	m_sharedData.Screen.Shaders.BloomBlur.setParameter("resolution", sf::Vector2f(1.f / (m_sharedData.Screen.ViewSize.x * m_blurmapScale),
														1.f / (m_sharedData.Screen.ViewSize.y * m_blurmapScale)));

}

///------threading------///
void BaseScreen::m_LaunchLoadingWindow(const std::string& message)
{
	//m_loadingSprite.setPosition(m_renderWindow.getView().getCenter());
	//m_loadingMessage.setString(message);
	//m_loadingMessage.setPosition(m_loadingSprite.getPosition().x - m_loadingMessage.getLocalBounds(). width / 2.f,
	//								m_loadingSprite.getPosition().y + 260.f);

	m_loadingString = message; //should really have a lock on this
	m_threadRunning = true;
	m_renderWindow.setActive(false);
	m_loadingThread.launch();
}

void BaseScreen::m_UpdateLoadingWindow()
{
	m_CreateLoadingDisplay();

	while(m_threadRunning)
	{
		m_loadingPosition.x += m_dtClock.restart().asSeconds();
		m_loadingShader.setParameter("position", m_loadingPosition);

		m_renderWindow.clear();
		m_renderWindow.pushGLStates();
		m_renderWindow.draw(m_loadingSprite, &m_loadingShader);
		m_renderWindow.draw(m_loadingMessage);
		//Console::Draw(m_renderWindow);
		m_renderWindow.popGLStates();
		m_renderWindow.display();

		//TODO try handling events
		sf::Event e;
		while(m_renderWindow.pollEvent(e)){};
	}
	m_renderWindow.setActive(false);
}

void BaseScreen::m_StopLoadingWindow()
{
	m_threadRunning = false;
	m_loadingThread.wait();
}

void BaseScreen::m_CreateLoadingDisplay()
{
	m_loadingTextureDiffuse = m_sharedData.Textures.Get("assets/textures/ui/loading_diffuse.png");
	m_loadingTextureDiffuse.setRepeated(true);
	m_loadingTextureDiffuse.setSmooth(true);
	m_loadingSprite = sf::Sprite(m_loadingTextureDiffuse);
	m_loadingSprite.setOrigin(m_loadingTextureDiffuse.getSize().x / 4.f, m_loadingTextureDiffuse.getSize().y / 4.f);
	m_loadingSprite.setScale(2.f, 2.f);
	m_loadingSprite.setPosition(m_renderWindow.getView().getCenter());

	m_loadingMessage = sf::Text(m_loadingString, m_sharedData.Fonts.Get("assets/fonts/default.ttf"), 40u);
	m_loadingMessage.setPosition(m_loadingSprite.getPosition().x - m_loadingMessage.getLocalBounds(). width / 2.f,
									m_loadingSprite.getPosition().y + 260.f);

}


std::array<std::string, 20> BaseScreen::m_randomMessages = {
"exec COMMAND.COM",
"Smuggling Raisins since 1903",
"Pay no attention to the man behind the curtain",
"Who cut the cheese?",
"You are not reading this",
"Always assume a wink is suspicious",
"Mauve is the new purple",
"Pet your kitty",
"There are no pork pies in the Netherlands",
"Sponsored by Powdered Toast",
"Please stop looking at me",
"Now with added Bosons!",
"Goes great with Salmon",
"Wouldn't you like to be a racer too?",
"Not tested on animals",
"Any bugs you may encounter should be considered a feature",
"You may have read this before",
"The chances are the odds of something happening",
"Mustard is underrated",
"Nothing up our sleeves"};
