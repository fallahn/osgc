///class for creating a UI component which looks like like an analogue VU meter//
#ifndef UI_METER_H_
#define UI_METER_H_

#include <Game/UIComponent.h>
#include <Game/LineSegment.h>
#include <SFML/Graphics/Sprite.hpp>
#include <array>

namespace Game
{
	class UIMeterSprite final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UIMeterSprite> Ptr;
		UIMeterSprite(const sf::Texture& front, const sf::Texture& back, const sf::Texture& needle, sf::Uint8 itemCount = 3u);

		void Update(float dt);

		sf::Uint8 SelectedIndex() const;

		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;
		sf::Vector2f GetSize() const;
		void SetIndexEnabled(sf::Uint8 index, bool enabled);
		void SetNeedlePosition(const sf::Vector2f& pos);
		void SelectIndex(sf::Uint8 index);
		
	private:

		enum State
		{
			MeterNormal = 0u,
			MeterSelected = 1u
		};

		sf::Uint8 m_selectedIndex, m_destinationIndex;
		sf::Uint8 m_itemCount;
		std::vector<bool> m_enabledItems;

		const sf::Texture& m_frontTexture;
		const sf::Texture& m_backTexture;
		std::array<sf::IntRect, 2u> m_subRects;
		sf::Sprite  m_frontSprite;
		sf::Sprite m_backSprite;

		LineSegment m_needle;
		float m_destAngle;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		void m_SetDestinationAngle();
	};
}
#endif