//animated sprite class based on - http://www.sfml-dev.org/wiki/en/sources/anisprite

#ifndef _ANISPRITE_H_
#define _ANISPRITE_H_

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include<SFML/System/Clock.hpp>

namespace Game
{
	class AniSprite : public sf::Sprite
	{
	public:
		AniSprite();

		AniSprite(const sf::Texture& Texture, int frameWidth, int frameHeight);
		~AniSprite();

		//returns a rectangle representing the current frame. Isn't this the same as sf::Sprite::getTextureRect() ?
		sf::IntRect getFramePosition(int frame) const;
		//returns the number of frames in the sprite sheet
		int getFrameCount() const;
		//returns a vector containing the width and height of a frame
		sf::Vector2i getFrameSize() const;
		//returns the current frame number, indexed from 0
		int getCurrentFrame() const;
		//sets the frame size explicitly
		void setFrameSize(int frameWidth, int frameHeight);
		//sets the currently selected frame
		void setFrame(int frame);
		//set the frame rate in frames per second
		void setFrameRate(float fps);
		//plays the sprite animation from the currently selected frame looped indefinitely
		void play();
		//plays a specific animation range. If the start frame is higher than the end frame then the animation is played reversed
		void play(int start, int end, bool looped = true);
		//stops the currently playing animation. Must be called when switching animation ranges
		void stop();
		//updates the sprite animation. Must be called at least once a frame
		void update();
		//returns true if the sprite is currently playing an animation
		bool playing() const;

	private:
		sf::Clock m_clock;
		float m_fps;
		bool m_playing, m_looped;
		int m_loopStart, m_loopEnd, m_currentFrame, m_frameWidth, m_frameHeight;
	};
};

#endif