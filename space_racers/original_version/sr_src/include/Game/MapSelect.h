///creates a 'carousel' of available maps from a map list///
#ifndef MAP_SELECT
#define MAP_SELECT

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/System/NonCopyable.hpp>

#include <Game/resourcemanager.h>
#include <list>

namespace Game
{
	class MapSelection : public sf::Drawable, private sf::NonCopyable
	{
	public:
		MapSelection(const sf::Font& font);
		void Create(const std::list<std::string>& mapList, TextureResource& textureResource);
		void Update(float dt);
		const std::string GetSelectedValue() const;
		bool Visible() const{return m_visible;};
		void SetVisible(bool visible = true){m_visible = visible;};
		void SetCentre(const sf::Vector2f& centre);
		const sf::FloatRect GetVisibleBounds() const
		{
			return sf::FloatRect(m_backgroundBox.getPosition(), m_backgroundBox.getSize());
		}
		bool Animating() const{return m_animating;};
		const sf::Text& GetMapname() const{return m_mapText;};

		bool Contains(const sf::Vector2f& point);
		void Activate();
		void Select();
		void Deselect();

	private:
		//vector of sprites for thumbnails
		std::vector< std::pair<std::string, std::unique_ptr<sf::Sprite> > > m_thumbSprites;
		//sprites for prev / next
		sf::Sprite m_prevSprite, m_nextSprite;
		sf::Sprite m_backgroundSprite;
		sf::Texture m_arrowTexture;
		//text for selected map name
		sf::Text m_mapText;
		//background box
		sf::RectangleShape m_backgroundBox;
		//indices of selected map sprite
		sf::Uint16 m_currentIndex, m_nextIndex;
		//scroll speed
		float m_scrollSpeed;
		const float m_initalScrollSpeed;
		bool m_animating;
		float m_travelledDistance;

		bool m_visible;
		//bool m_mouseDown;
		//bounding rectangle
		sf::FloatRect m_bounds;
		//spacing between icons
		const float m_iconSpacing;
		//total size of all icons in a row
		float m_iconsWidth;



		//override for drawable
		void draw(sf::RenderTarget& rt, sf::RenderStates state) const;
		//centres new text
		void m_SetText();

		//event handlers
		void m_OnPrevClick();
		void m_OnNextClick();

		//hackery for UI wrapper		
		enum Selection
		{
			Nothing,
			Left,
			Right
		}m_selection;

	};
};

#endif
