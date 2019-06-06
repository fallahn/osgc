//ui component representing a togglable checkbox / bullet//
#ifndef UI_CHECK_H_
#define UI_CHECK_H_

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>

#include <Game/UIComponent.h>
#include <array>

namespace Game
{
	class UICheckBox final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UICheckBox> Ptr;
		enum class Alignment
		{
			Left,
			Right
		};

		UICheckBox(const sf::Font& font, const sf::Texture& texture);

		void SetText(const std::string& text);
		void SetTextSize(sf::Uint8 size);
		bool Checked() const;
		void Check(bool b = true);
		void SetAlignment(UICheckBox::Alignment alignment);

		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();
		void Deactivate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;

	private:
		enum State
		{
			Normal = 0u,
			Selectected = 1u,
			CheckedNormal = 2u,
			CheckedSelected = 3u
		};
		const sf::Texture& m_texture;
		sf::Sprite m_sprite;
		sf::Text m_text;
		std::array<sf::IntRect, 4u> m_subrects;
		bool m_checked;
		Alignment m_alignment;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		void m_AlignText();
	};
}

#endif