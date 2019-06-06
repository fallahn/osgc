///defines a track node used for navigation///
#ifndef TRACKNODE_H_
#define TRACKNODE_H_

#include <SFML/System/Vector2.hpp>

namespace Game
{
	struct TrackNode
	{
		TrackNode() : ID(0), SpawnAngle(0.f){};
		TrackNode(sf::Uint8 id, const sf::Vector2f position)
			: ID(id), Position(position), SpawnAngle(0.f){};
		sf::Uint8 ID;
		sf::Vector2f Position, NextUnit; //normalised vector to next node
		float NextMagnitude; //distance to next node
		float SpawnAngle; //angle by which to rotate vehicle when spawning at this point
	};
};

#endif