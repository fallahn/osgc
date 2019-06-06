///base class for game objects///
/*All non-terrain entities (such as players, NPCs items and weapons) are derived from this class which provides
the basic texture properties and its unique ident within the engine*/
#ifndef _BaseEntity_H_
#define _BaseEntity_H_

#include <SFML/System/Vector2.hpp>

#include <Game/Common.h>
#include <Game/AnimatedSprite.h>

#include <Helpers.h>

#include <memory>

namespace Game
{
	class BaseEntity
	{
	public:
		BaseEntity();
		virtual ~BaseEntity(){};
		//returns a reference to the entity's sprite
		virtual AniSprite& GetSprite();
		//the unique ID of the enitity. The upper byte is the group id and lower the unique id within the group
		//mask this with GROUP_ID and UID to find its respective values
		GroupId Ident;
		//gets the current state of the entity
		const State GetState() const;
		//sets the current state of the entity
		void SetState(State state);
		//Gets the centre point of the object in world coords
		const sf::Vector2f GetCentre() const;

	protected:
		sf::Texture m_texture;
		AniSprite m_sprite;
		State m_state;
	};
};

#endif