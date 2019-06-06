///creates a label for the UI system///
#ifndef UI_LABEL_H_
#define UI_LABEL_H_

#include <Game/UIComponent.h>
#include <SFML/Graphics/Text.hpp>

namespace Game
{
	class UILabel : public UIComponent
	{
	public:
		typedef std::shared_ptr<UILabel> Ptr;

		UILabel(const std::string& text, const sf::Font& font);

		virtual bool Selectable() const;
		void SetText(const std::string& text);

		virtual void HandleEvent(const sf::Event& e);


	private:
		sf::Text m_text;
		virtual void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}

#endif