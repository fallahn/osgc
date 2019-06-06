////source for console for game engine////
#include <Game/Console.h>


///-------Logging Class -------///

Console::Log::Log()
		: m_logFile		(std::ofstream("console.log", std::ios_base::app)),
	m_consoleAvailable	(true),
	m_maxBufferLines	(40u)
{
	//opened the file for logging so add a date to say where we started
	time_t timeNow = time(0);
	tm* localTime = localtime(&timeNow);
	char buffer[40];
	strftime(buffer, 40, "Log started on: %x %X", localTime);
	m_logFile << std::endl << buffer << std::endl;

	if(!m_font.loadFromFile("assets/fonts/console.ttf"))
	{
		std::cout << "Console font not loaded, console unavailable" << std::endl;
		m_consoleAvailable = false;
	}
	else
	{
		m_background.setSize(sf::Vector2f(1440.f, 1000.f));
		m_background.setFillColor(sf::Color(0u, 0u, 0u, 180u));
		m_text.setFont(m_font);
		m_text.setCharacterSize(24u);

		std::cout << "Using engine console, press tab for more verbose output" << std::endl;
	}
}

void Console::Log::m_Update()
{
	if(!m_consoleAvailable) return; //font is missing
	
	//push back new line onto buffer
	std::string s;
	std::getline(m_stringStream, s);
	m_outputBuffer.push_back(s);

	//pop front if over size
	if(m_outputBuffer.size() > m_maxBufferLines)
		m_outputBuffer.pop_front();

	//update text with new string from buffer
	s.clear();
	for(auto line : m_outputBuffer)
		s += line + "\n";

	m_text.setString(s);
}

void Console::Log::draw(sf::RenderTarget& rt, sf::RenderStates states)const
{
	if(!m_consoleAvailable) return; //font is missing
	rt.draw(m_background);
	rt.draw(m_text);
}


//---static console members---//
Console::Log Console::lout;
bool Console::Show = false;
std::map<std::string, std::function<void(std::string)> > Console::m_convars;
void Console::Draw(sf::RenderTarget& rt)
{
	if(!Show) return;
	sf::View oldView, newView;
	oldView = newView = rt.getView();
	newView.setCenter(newView.getSize() / 2.f);

	rt.setView(newView);
	rt.draw(lout);
	rt.setView(oldView);
}

void Console::AddConvar(std::string command, void (*func)(std::string))
{
	//adds pointer to command function to command map
	m_convars[command] = func;
}