///header for scroll sprite UI component//
#ifndef UI_SCROLL_SPRITE_H_
#define UI_SCROLL_SPRITE_H_

#include <Game/UIButton.h>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/System/Clock.hpp>

namespace Game
{
	class UIScrollSprite final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UIScrollSprite> Ptr;

		UIScrollSprite(const sf::Texture& front, const sf::Texture& back);

		void Update(float dt); //updates the animations

		void AddItem(const sf::Texture& texture, const std::string& title, const sf::Font& font);
		void ClearItems();

		sf::Uint16 SelectedIndex() const;

		//implements base functions
		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;

		void SetIndexEnabled(sf::Uint16 index, bool state);
		void SelectIndex(sf::Uint16 index);
		//offset from component centre
		void SetItemOffset(const sf::Vector2f& offset);
		void SetTextOffset(const sf::Vector2f& offset);
		void SetTextSize(sf::Uint16 size);
	private:

		enum State
		{
			ScrollNormal = 0u,
			ScrollSelected = 1u
		};

		sf::Uint16 m_selectedIndex;
		sf::Uint16 m_destinationIndex;

		struct Item
		{
			Item(const sf::Texture& texture, const std::string& title, const sf::Font& font, sf::Uint16 textSize);
			sf::Sprite sprite;
			sf::Text text;
			bool visible;
			bool enabled;
		};
		std::vector<Item> m_items;
		sf::Vector2f m_itemOffset;
		sf::Uint16 m_textSize;
		sf::Vector2f m_textOffset;

		const sf::Texture& m_frontTexture;
		const sf::Texture& m_backTexture;
		std::array<sf::IntRect, 2u> m_subRects;
		sf::Sprite m_frontSprite;
		sf::Sprite m_backSprite;

		sf::Shader m_scanShader;
		sf::Shader m_ghostShader;
		sf::Clock m_scanClock;
		sf::Clock m_ghostClock;

		void draw(sf::RenderTarget& rt, sf::RenderStates state) const;
	};
}

#endif