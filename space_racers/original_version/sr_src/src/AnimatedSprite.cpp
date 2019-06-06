//source for animated sprite class
#include <Game/AnimatedSprite.h>

using namespace Game;


AniSprite::AniSprite()
	: sf::Sprite(),
	m_fps(1.f),
	m_currentFrame(0),
	m_playing(false),
	m_loopStart(0)
{
	setFrameSize(0, 0);
}

AniSprite::AniSprite(const sf::Texture& Texture, int frameW, int frameH)
		: sf::Sprite(Texture),
	m_fps(1.f),
	m_currentFrame(0),
	m_playing(false),
	m_loopStart(0)
{
	setFrameSize(frameW, frameH);
}

AniSprite::~AniSprite()
{
}

int AniSprite::getFrameCount() const
{
	unsigned int across =
		getTexture()->getSize().x /
		m_frameWidth;
	unsigned int down =
		getTexture()->getSize().y /
		m_frameHeight;

	return across * down;
}
//first frame is frame ZERO
sf::IntRect AniSprite::getFramePosition(int frame) const
{
	//get number across and down
	unsigned int across =
		getTexture()->getSize().x /
		m_frameWidth;
	unsigned int down =
		getTexture()->getSize().y /
		m_frameHeight;

	int tileY = frame / across ; //get Y on grid
	int tileX = frame % across ; //get X on grid

	return sf::IntRect(tileX * m_frameWidth,
						tileY * m_frameHeight,
						m_frameWidth,
						m_frameHeight);
}

sf::Vector2i AniSprite::getFrameSize() const
{
	return sf::Vector2i(m_frameWidth, m_frameHeight);
}

void AniSprite::setFrameSize(int frameW, int frameH)
{
	m_frameWidth = frameW;
	m_frameHeight = frameH;
	setTextureRect(sf::IntRect(0, 0, frameW, frameH));
}

void AniSprite::setFrame(int frame)
{
	m_currentFrame = frame;
	setTextureRect(getFramePosition(m_currentFrame));
}

void AniSprite::setFrameRate(float newfps)
{
	m_fps = newfps;
}

void AniSprite::play()
{
	play(0, getFrameCount() - 1);
}
void AniSprite::play(int start, int end, bool looped)
{
	if(!m_playing && end <= getFrameCount())
	{
		m_loopStart = start;
		m_loopEnd = end;
		//m_currentFrame = start; //comment out to play from current frame
		m_playing = true;
		m_clock.restart();
		m_looped = looped;
	}
}

void AniSprite::stop()
{
	m_playing = false;
}

void AniSprite::update()
{
	if(m_playing)
	{
 		float timePosition = (m_clock.getElapsedTime().asSeconds() * m_fps);
		int frameCount;
		if(m_loopStart < m_loopEnd)
		{
			frameCount = (m_loopEnd + 1) - m_loopStart;
			m_currentFrame = m_loopStart + static_cast<int>(timePosition) % frameCount;
		}
		else
		{
			frameCount = (m_loopStart + 1) - m_loopEnd;
			m_currentFrame = m_loopStart - static_cast<int>(timePosition) % frameCount;
		}

		//printf("%f:%i\n", m_clock.getElapsedTime(), m_currentFrame);

		setTextureRect(getFramePosition(m_currentFrame));
		if(!m_looped && m_currentFrame == m_loopEnd)
			m_playing = false;
	}
}

bool AniSprite::playing() const
{
	return m_playing;
}

int AniSprite::getCurrentFrame() const
{
	return m_currentFrame;
}
