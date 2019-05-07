#pragma once

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

#include <utility>

using Segment = std::pair<sf::Vector2f, sf::Vector2f>;

bool intersects(const Segment& segOne, const Segment& segTwo, sf::Vector2f& intersection);

struct IntersectionResult
{
    bool intersects = false;
    sf::Vector2f normal;
    sf::Vector2f intersectionPoint;
    float penetration = 0.f;
};

IntersectionResult intersects(const Segment&, sf::FloatRect target);