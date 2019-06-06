////source code for map selection carousel///
#include <SFML/Window/Mouse.hpp>

#include <Game/MapSelect.h>

using namespace Game;

namespace
{
	const sf::Color lowlight(120u, 120u, 120u);
}

//ctor
MapSelection::MapSelection(const sf::Font& font)
	: m_currentIndex		(0u),
	m_mapText				("", font, 60u),
	m_nextIndex				(m_currentIndex),
	m_initalScrollSpeed		(500.f),
	m_travelledDistance		(0.f),
	m_animating				(false),
	m_visible				(false),
	m_selection				(Nothing),
	m_bounds				(0.f, 0.f, 1000.f, 1080.f),
	m_iconSpacing			(m_bounds.width / 3.f),
	m_iconsWidth			(0.f)
{
}

///public///
void MapSelection::Create(const std::list<std::string>& mapList, TextureResource& textureResource)
{
	//create thumbnail sprites
	for(const auto& map : mapList)
	{
		std::unique_ptr<sf::Sprite> sprite(new sf::Sprite(textureResource.Get("assets/maps/thumbs/" + map + ".png")));
		sprite->setOrigin(static_cast<float>(sprite->getTextureRect().width / 2), static_cast<float>(sprite->getTextureRect().height / 2));
		m_thumbSprites.push_back(std::make_pair(map, std::move(sprite)));
		m_iconsWidth += m_iconSpacing;
	}

	m_arrowTexture = textureResource.Get("assets/textures/ui/menu_arrow.png");

	m_nextSprite.setTexture(m_arrowTexture);
	m_nextSprite.setOrigin(0.f, static_cast<float>(m_arrowTexture.getSize().y) / 2.f);
	m_prevSprite.setTexture(m_arrowTexture);
	m_prevSprite.setOrigin(0.f, m_nextSprite.getOrigin().y);
	//flip prev button
	m_prevSprite.setScale(-1.f, 1.f);

	//set up box for background
	m_backgroundBox.setSize(sf::Vector2f(m_bounds.width, 640.f)); //need to make this magic number tie in with tallest sprite
	m_backgroundBox.setFillColor(sf::Color(0u, 0u, 0u, 120u));
	m_backgroundBox.setOutlineThickness(-2.f);
	m_backgroundBox.setOutlineColor(sf::Color(153u, 153u, 153u, 120u));

	//perform initial layout
	SetCentre(sf::Vector2f(720.f, 440.f));
}

void MapSelection::Update(float dt)
{	
	//-----update buttons-----//
	//bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Left);
	//sf::Color lowLight(120u, 120u, 120u);
	//
	////highlight arrows when mouse over
	//if(m_nextSprite.getGlobalBounds().contains(mousePos))
	//{
	//	m_nextSprite.setColor(sf::Color::White);
	//	//handle click
	//	if(mouseDown)
	//	{
	//		m_nextSprite.setColor(lowLight);
	//		if(!m_mouseDown)
	//			m_OnNextClick();
	//	}
	//}
	//else
	//{
	//	m_nextSprite.setColor(lowLight);
	//}

	//if(m_prevSprite.getGlobalBounds().contains(mousePos))
	//{
	//	m_prevSprite.setColor(sf::Color::White);
	//	//handle click
	//	if(mouseDown)
	//	{
	//		if(!m_mouseDown)m_OnPrevClick();
	//		m_prevSprite.setColor(lowLight);
	//	}
	//}
	//else
	//{
	//	m_prevSprite.setColor(lowLight);
	//}

	//m_mouseDown = mouseDown;
	//---------------------------------//


	//------update icon animation------//
	const float reduction = 200.f * dt;
	if(m_scrollSpeed > reduction)
		m_scrollSpeed -= reduction;
	else if(m_scrollSpeed < -reduction)
		m_scrollSpeed += reduction;
	
	const float moveSpeed = m_scrollSpeed * dt;
	m_travelledDistance += abs(moveSpeed);
	for(auto& sprite : m_thumbSprites)
	{
		//update position
		if(m_currentIndex != m_nextIndex)
			sprite.second->move(moveSpeed, 0.f);
		
		//keep sprites in bounds
		if(sprite.second->getPosition().x > m_bounds.left + m_iconsWidth)
			sprite.second->move(-m_iconsWidth, 0.f);
		else if(sprite.second->getPosition().x < m_bounds.left)
			sprite.second->move(m_iconsWidth, 0.f);

		//update scale
		const float boundsCentre = m_bounds.left + (m_bounds.width / 2);
		const float relPos = sprite.second->getPosition().x - boundsCentre;
		const float scale = 1.f - abs(relPos / (m_bounds.width / 2.f));
		sprite.second->setScale(scale, scale);

		//update colour
		const float colour = 255.f * scale;
		const sf::Uint8 c = static_cast<sf::Uint8>(colour);
		sprite.second->setColor(sf::Color(c, c, c));

		//stop anim if in right place
		if(relPos > -6.f && relPos < 6.f
			&& m_travelledDistance > 50.f)
		{
			m_travelledDistance = 0.f;
			m_scrollSpeed = 0.f;
			m_currentIndex = m_nextIndex;
			m_animating = false;

			//set title
			m_SetText();
		}
	}
}

const std::string MapSelection::GetSelectedValue() const
{
	return m_thumbSprites[m_currentIndex].first;
}

void MapSelection::SetCentre(const sf::Vector2f& centre)
{	
	//lays out the components of the selection
	m_bounds.left = centre.x - (m_bounds.width / 2.f);
	m_bounds.top = centre.y - (m_bounds.height / 2.f);

	const float yPos = centre.y * 0.66f;
	const float xOffset = m_bounds.width / 2.f;
	m_nextSprite.setPosition(centre.x + xOffset, yPos);
	m_prevSprite.setPosition(centre.x - xOffset, yPos);

	for(auto i = 0u; i < m_thumbSprites.size(); i++)
	{
		m_thumbSprites[i].second->setPosition((static_cast<float>(i) * m_iconSpacing) + centre.x, yPos);
		//offset by selected index
		m_thumbSprites[i].second->move(-m_iconSpacing * static_cast<float>(m_currentIndex), 0.f);
	}

	m_backgroundBox.setPosition(m_bounds.left, yPos - (m_backgroundBox.getSize().y / 2.f));

	//set text *after* positioning box
	m_SetText();
}

bool MapSelection::Contains(const sf::Vector2f& point)
{
	if(m_nextSprite.getGlobalBounds().contains(point))
	{
		m_nextSprite.setColor(sf::Color::White);
		m_prevSprite.setColor(lowlight);
		m_selection = Right;
		return true;
	}
	else if(m_prevSprite.getGlobalBounds().contains(point))
	{
		m_prevSprite.setColor(sf::Color::White);
		m_nextSprite.setColor(lowlight);
		m_selection = Left;
		return true;
	}
	m_prevSprite.setColor(lowlight);
	m_nextSprite.setColor(lowlight);
	return false;
}

void MapSelection::Activate()
{
	if(m_selection == Right)
	{
		m_OnNextClick();
		m_nextSprite.setColor(lowlight);
	}
	else if(m_selection == Left)
	{
		m_OnPrevClick();
		m_prevSprite.setColor(lowlight);
	}
}

///private///
void MapSelection::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	if(!m_visible) return;

	rt.draw(m_backgroundBox);

	for(auto& i : m_thumbSprites)
		if(m_bounds.contains(i.second->getPosition()))
			rt.draw(*i.second);

	rt.draw(m_prevSprite);
	rt.draw(m_nextSprite);
	//rt.draw(m_mapText); //draw this out separately to avoid bloom
}

void MapSelection::m_SetText()
{
	//set title
	m_mapText.setString(m_thumbSprites[m_currentIndex].first);
	const float textCentre = m_mapText.getLocalBounds().width / 2.f;
	m_mapText.setPosition(((m_bounds.width / 2.f) + m_bounds.left) - textCentre, m_backgroundBox.getPosition().y + m_backgroundBox.getSize().y/* + 10.f*/);
}


//event handlers//
void MapSelection::m_OnNextClick()
{
	if(m_animating) return;
	m_nextIndex = m_currentIndex - 1u;
	if(m_nextIndex > m_thumbSprites.size()) //unsigned flip over to MAX
		m_nextIndex = m_thumbSprites.size() - 1u;

	//set velocity
	m_scrollSpeed = m_initalScrollSpeed;
	m_animating = true;
}

void MapSelection::m_OnPrevClick()
{
	if(m_animating) return;
	m_nextIndex = m_currentIndex + 1u;
	if(m_nextIndex == m_thumbSprites.size())
		m_nextIndex = 0u;

	//set velocity
	m_scrollSpeed = -m_initalScrollSpeed;
	m_animating = true;
}

