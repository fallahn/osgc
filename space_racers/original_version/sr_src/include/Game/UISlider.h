///creates a UIComponent slider with range 0 - X///
#ifndef SLIDER_H_
#define SLIDER_H_

#include <Game/UIComponent.h>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Text.hpp>

#include <array>

namespace Game
{
	class UISlider final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UISlider> Ptr;
		enum class Direction
		{
			Horizontal,
			Vertical
		};

		UISlider(const sf::Font& font, const sf::Texture& t, float length = 250.f, float maxValue = 100.f);

		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();
		void Deactivate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;

		void Update(float dt);

		//-------------------//

		void SetMaxValue(float value);
		void SetDirection(UISlider::Direction d);
		void SetSize(float length);
		float GetValue() const;

		void SetText(const std::string& s);
		void SetTextSize(sf::Uint16 size);
		void SetValue(float val);

	private:
		float m_maxValue;
		float m_length;
		Direction m_direction;
		sf::FloatRect m_localBounds;

		sf::Sprite m_handleSprite;
		sf::RectangleShape m_slotShape;
		enum State
		{
			Normal = 0u,
			Selected = 1u
		};
		std::array<sf::IntRect, 2u> m_subrects;
		mutable sf::Vector2f m_mousePos;

		const sf::Font& m_font;
		sf::Text m_text;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
		void m_UpdateText();
	};
}

#endif