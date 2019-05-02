#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <utility>

using Segment = std::pair<sf::Vector2f, sf::Vector2f>;

bool intersects(const Segment& segOne, const Segment& segTwo, sf::Vector2f& intersection);

//returns distance to intersect and normal vector of intersection
std::pair<sf::Vector2f, sf::Vector2f> getDistance(sf::Vector2f point, sf::FloatRect target);