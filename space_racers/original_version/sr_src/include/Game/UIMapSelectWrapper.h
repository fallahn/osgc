///hacky class to wrap the map selection oojit in a UIComponent///
//because I'm too lazy to rewrite this properly//

#ifndef MAP_WRAP_H_
#define MAP_WRAP_H_

#include <Game/UIComponent.h>

namespace Game
{
	class MapSelection;

	class UIMapSelectWrapper final : public UIComponent
	{
	public:
		typedef std::shared_ptr<UIMapSelectWrapper> Ptr;
		UIMapSelectWrapper(MapSelection& ms);

		bool Selectable() const;
		void Select();
		void Deselect();

		void Activate();
		void Deactivate();

		void HandleEvent(const sf::Event& e);

		bool Contains(const sf::Vector2f& point) const;


	private:

		MapSelection& m_mapSelection;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;

	};
}

#endif //MAP_WRAP_H_