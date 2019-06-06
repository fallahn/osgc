////class specifically for animating screen while inital assets are loaded///
#ifndef LOADING_DISPLAY_H_
#define LOADING_DISPLAY_H_

#include <SFML/System/Clock.hpp>
#include<SFML/Graphics/RenderWindow.hpp>

#include <atomic>

class LoadingDisplay
{
public:
	LoadingDisplay(sf::RenderWindow& rw)
		:	m_rw		(rw),
		m_clearColour	(255.f),
		Running			(true)
	{

	}

	void ThreadFunc()//
	{
		while(Running)
		{
			
			float val = 70.f * m_clock.restart().asSeconds();
			if(m_clearColour > val)
			{
				m_clearColour -= val;
			}
			sf::Uint8 c = static_cast<sf::Uint8>(m_clearColour);
			m_rw.clear(sf::Color(c, c, c));
			m_rw.display();
		}
		m_rw.setActive(false);
	}

	std::atomic<bool> Running;
private:

	sf::RenderWindow& m_rw;
	float m_clearColour;
	sf::Clock m_clock;
};

#endif