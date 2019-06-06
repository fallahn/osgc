///implements a combobox UI component///
#ifndef UI_COMBO_H_
#define UI_COMBO_H_

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <Game/UIComponent.h>
#include <vector>

namespace Game
{
	class UIComboBox final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UIComboBox> Ptr;

		UIComboBox(const sf::Font& font, const sf::Texture& texture);

		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();
		void Deactivate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;
		
		//--------//

		void AddItem(const std::string& text, sf::Int32 val);
		const std::string& GetSelectedText() const;
		sf::Int32 GetSelectedValue() const;

		void SetBackgroundColour(const sf::Color& colour);
		void SetHighlightColour(const sf::Color& colour);
		void SetSelectedIndex(sf::Uint16 index);
		sf::Uint32 Size() const;
		void SelectItem(const std::string& s);
		void SelectItem(sf::Uint16 val);

	private:
		struct Item
		{
			typedef std::unique_ptr<Item> Ptr;
			std::string Text;
			sf::Int32 Value;

			sf::FloatRect Bounds;
			sf::Text ItemText;

			Item(const std::string& s, sf::Int32 v, const sf::Font& font);
		};
		
		std::vector<Item::Ptr> m_items;

		bool m_showItems;
		sf::RectangleShape m_mainShape;
		sf::RectangleShape m_dropdownShape;
		mutable sf::RectangleShape m_highlightShape;

		sf::Uint16 m_selectedIndex;
		sf::Text m_selectedText;

		mutable sf::Vector2f m_mousePoint;

		const sf::Font& m_font;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;

	};
}

#endif