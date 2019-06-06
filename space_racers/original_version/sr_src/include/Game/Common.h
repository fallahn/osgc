#ifndef _COMMON_H_
#define _COMMON_H_

#include <SFML/Graphics/Color.hpp>

enum Direction
{
	Up,
	Down,
	Left,
	Right
};

enum State
{
	Alive,
	Dying,
	Dead,
	Falling,
	Eliminated,
	Celebrating
};

enum ControlSet
{
	KeySetOne = 0u,
	KeySetTwo = 1u,
	ControllerOne = 2u,
	ControllerTwo = 3u,
	CPU = 4u,
	Network = 5u
};

enum Intelligence
{
	Human,
	Smart,
	Average,
	Dumb
};

enum RoundState
{
	Racing,
	RoundEnd,
	RoundReset,
	RaceWon,
};

//group IDs of BaseEntity derived class
//see BaseEntity.h for explanation
typedef sf::Uint16 GroupId;
typedef sf::Uint16 TypeId;
//used for masking IDs
const GroupId GROUP_ID = 0xff00;
const GroupId UID = 0x00ff;
//IDs of types
const TypeId IdPlayer = 0x0100;
const TypeId IdWeapon = 0x0200;

namespace Game
{
	struct PlayerColours
	{
		static const sf::Color Light(sf::Uint8 index)
		{
			switch(index)
			{
			default:
			case 0:
				return sf::Color(255u, 0u, 10u);
			case 1:
				return sf::Color(0u, 127u, 255u);
			case 2:
				return sf::Color(63u, 0u, 255u);
			case 3:
				return sf::Color(0u, 191u, 127u);
			}
		}

		static const sf::Color Dark(sf::Uint8 index)
		{
			switch(index)
			{
			default:
			case 0:
				return sf::Color(76u, 0u, 0u);
			case 1:
				return sf::Color(0u, 37u, 76u);
			case 2:
				return sf::Color(18u, 0u, 76u);
			case 3:
				return sf::Color(0u, 54u, 37u);
			}
		}
	};
}

#endif