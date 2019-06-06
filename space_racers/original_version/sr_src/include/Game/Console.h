////header file for engine console///
#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <Game/Common.h>
#include <sstream>
#include <fstream>
#include <list>
#include <functional>


class Console : private sf::NonCopyable
{
private:
	class Log : private sf::Drawable, private sf::NonCopyable
	{
	public:
		friend Console;
		Log();
		~Log()
		{
			//m_logFile.close();
		}
		//templated overload to stream all types supported by ostream
		template <class T>
		Log& operator << (T info)
		{
			m_logFile << info;
			m_stringStream << info;
			return *this;
		}
		//overload to allow mutator methods (endl etc)
		Log& operator << (std::ostream& (*mut)(std::ostream&))
		{
			m_logFile << mut;
			m_stringStream << mut;
			m_Update();
			return *this;
		}

	private:
		std::ofstream m_logFile;
		std::stringstream m_stringStream;

		//members for drawing console type interface on screen
		void m_Update(); //should this just be called by the << operator?
		sf::Text m_text;
		sf::Font m_font;
		sf::RectangleShape m_background;
		bool m_consoleAvailable;
		std::list<std::string> m_outputBuffer;
		const sf::Uint8 m_maxBufferLines;

		void draw(sf::RenderTarget& rt, sf::RenderStates state) const;
	};

	//map of console commands
	static std::map< std::string, std::function<void (std::string)> > m_convars;

public:
	static Log lout;
	static bool Show;
	static void Draw(sf::RenderTarget& rt);
	static void AddConvar(std::string command, void (*func)(std::string));
};

#endif