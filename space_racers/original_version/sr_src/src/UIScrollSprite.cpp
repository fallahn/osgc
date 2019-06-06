///source for scroll sprite ui  component///
#include <Game/UIScrollSprite.h>
#include <Game/Shaders/ScanLines.h>
#include <Helpers.h>
#include <SFML/Graphics/RenderTarget.hpp>

#include <cassert>

using namespace Game;

//ctor
UIScrollSprite::UIScrollSprite(const sf::Texture& front, const sf::Texture& back)
	: m_selectedIndex	(0u),
	m_destinationIndex	(0u),
	m_frontTexture		(front),
	m_backTexture		(back),
	m_textSize			(40u)
{
	m_backSprite.setTexture(m_backTexture, true);
	sf::IntRect subRect(0, 0, back.getSize().x, back.getSize().y / 2u);
	m_subRects[State::ScrollNormal] = subRect;
	subRect.top += subRect.height;
	m_subRects[State::ScrollSelected] = subRect;
	m_backSprite.setTextureRect(m_subRects[State::ScrollNormal]);
	m_backSprite.setOrigin(static_cast<float>(subRect.width / 2), static_cast<float>(subRect.height / 2));


	m_frontSprite.setTexture(m_frontTexture);
	m_frontSprite.setOrigin(sf::Vector2f(static_cast<sf::Vector2f>(front.getSize() / 2u)));

	m_scanShader.loadFromMemory(scanlines, sf::Shader::Fragment);
	m_ghostShader.loadFromMemory(ghost, sf::Shader::Fragment);
}

//public
bool UIScrollSprite::Selectable() const
{
	return true;
}

void UIScrollSprite::Select()
{
	UIComponent::Select();
	m_backSprite.setTextureRect(m_subRects[State::ScrollSelected]);
}

void UIScrollSprite::Deselect()
{
	UIComponent::Deselect();
	m_backSprite.setTextureRect(m_subRects[State::ScrollNormal]);
}

void UIScrollSprite::Activate()
{
	if(!Enabled()) return;
	
	//move to next enabled selection index
	if(m_destinationIndex == m_selectedIndex)
	{		
		do
			m_destinationIndex = (m_destinationIndex + 1u) % m_items.size();
		while (!m_items[m_destinationIndex].enabled
			&& m_destinationIndex != m_selectedIndex);

		m_items[m_destinationIndex].visible = true;
	}
}

void UIScrollSprite::HandleEvent(const sf::Event& e)
{

}

bool UIScrollSprite::Contains(const sf::Vector2f& point) const
{
	return getTransform().transformRect(m_frontSprite.getGlobalBounds()).contains(point);
}

void UIScrollSprite::Update(float dt)
{
	if(m_destinationIndex != m_selectedIndex)
	{
		//rotate sprites
		const float distance = 180.f - m_items[m_selectedIndex].sprite.getRotation();
		float rotation = (10.f + (distance * 4.f)) * dt;
		m_items[m_selectedIndex].sprite.rotate(rotation);
		m_items[m_destinationIndex].sprite.rotate(rotation);

		//end rotation if destination is close enough to 0
		rotation = m_items[m_destinationIndex].sprite.getRotation();
		if(rotation > 358.f || rotation < 180.f)
		{
			m_items[m_selectedIndex].sprite.setRotation(180.f);
			m_items[m_selectedIndex].visible = false;
			m_items[m_destinationIndex].sprite.setRotation(0.f);
			m_items[m_destinationIndex].sprite.setColor(sf::Color::White);
			m_selectedIndex = m_destinationIndex;
		}
		
		//update transparency
		const float ratio = 255 / 180.f;
		sf::Uint8 a = static_cast<sf::Uint8>(ratio * rotation);
		m_items[m_destinationIndex].sprite.setColor(sf::Color(255u, 255u, 255u, a));
		m_items[m_destinationIndex].text.setColor(sf::Color(255u, 255u, 255u, a));

		a = static_cast<sf::Uint8>(ratio * m_items[m_selectedIndex].sprite.getRotation());
		m_items[m_selectedIndex].sprite.setColor(sf::Color(255u, 255u, 255u, 255u - a));
		m_items[m_selectedIndex].text.setColor(sf::Color(255u, 255u, 255u, 255u - a));
	}
	m_scanShader.setParameter("time", m_scanClock.getElapsedTime().asSeconds());

	if(m_ghostClock.getElapsedTime().asSeconds() > 0.2f)
	{
		if( Helpers::Random::Int(0, 40) == 0)
		{
			const float numX = Helpers::Random::Float(-0.03f, 0.03f);
			const float numY = Helpers::Random::Float(-0.03f, 0.03f);
			m_ghostShader.setParameter("ghostOffset", sf::Vector2f(numX, numY));
			m_ghostClock.restart();
		}
		else
		{
			m_ghostShader.setParameter("ghostOffset", sf::Vector2f());
		}
	}
}

void UIScrollSprite::AddItem(const sf::Texture& texture, const std::string& title, const sf::Font& font)
{
	Item item(texture, title, font, m_textSize);
	if(m_items.empty()) item.visible = true;
	else item.sprite.rotate(180.f);

	item.sprite.move(m_itemOffset);
	item.text.setPosition(item.sprite.getPosition());
	item.text.move(m_textOffset);
	m_items.push_back(item);
}

void UIScrollSprite::ClearItems()
{
	m_items.clear();
	m_selectedIndex = 0u;
}

sf::Uint16 UIScrollSprite::SelectedIndex() const
{
	return m_selectedIndex;
}

void UIScrollSprite::SelectIndex(sf::Uint16 index)
{
	assert(index < m_items.size());
	if(m_items[index].enabled)
	{
		m_destinationIndex = index;
		m_items[index].visible = true;
	}
}

void UIScrollSprite::SetIndexEnabled(sf::Uint16 index, bool state)
{
	assert(index < m_items.size());

	m_items[index].enabled = state;
}

void UIScrollSprite::SetItemOffset(const sf::Vector2f& offset)
{
	m_itemOffset = offset;
	for(auto& i : m_items)
		i.sprite.setPosition(offset);
}

void UIScrollSprite::SetTextOffset(const sf::Vector2f& offset)
{
	m_textOffset = offset;
	for(auto& i : m_items)
		i.text.setPosition(offset);
}

void UIScrollSprite::SetTextSize(sf::Uint16 size)
{
	m_textSize = size;
	for(auto& i : m_items)
	{
		i.text.setCharacterSize(size);
		Helpers::Position::CentreOrigin(i.text);
	}
}

//private
UIScrollSprite::Item::Item(const sf::Texture& texture, const std::string& title, const sf::Font& font, sf::Uint16 textSize)
	: visible	(false),
	enabled		(true),
	sprite		(texture),
	text		(title, font, textSize)
{
	Helpers::Position::CentreOrigin(sprite);
	Helpers::Position::CentreOrigin(text);
}

void UIScrollSprite::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	states.transform *= getTransform();

	rt.draw(m_backSprite, states);

	states.shader = &m_ghostShader;
	for(const auto& i : m_items)
	{
		if(i.visible)
		{
			rt.draw(i.sprite, states);
			rt.draw(i.text, states);
		}
	}
	states.blendMode = sf::BlendMultiply;
	states.shader = &m_scanShader;
	rt.draw(m_frontSprite, states);
}
