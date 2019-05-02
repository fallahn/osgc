#include "CollisionUtil.hpp"

//from Andre LaMothe's black art of game programming
bool intersects(const Segment& segOne, const Segment& segTwo, sf::Vector2f& intersection)
{
    sf::Vector2f dirOne = segOne.second - segOne.first;
    sf::Vector2f dirTwo = segTwo.second - segTwo.first;

    float s = (-dirOne.y * (segOne.first.x - segTwo.first.x) + dirOne.x * (segOne.first.y - segTwo.first.y)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);
    float t = (dirTwo.x * (segOne.first.y - segTwo.first.y) - dirTwo.y * (segOne.first.x - segTwo.first.x)) / (-dirTwo.x * dirOne.y + dirOne.x * dirTwo.y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1)
    {
        //collision detected
        intersection.x = segOne.first.x + (t * dirOne.x);
        intersection.y = segOne.first.y + (t * dirOne.y);
        return true;
    }

    return false;
}

IntersectionResult intersects(const Segment& firstSegment, sf::FloatRect target)
{
    sf::Vector2f targetCentre(target.left + (target.width / 2.f), target.top + (target.height / 2.f));
    auto point = firstSegment.first;

    //TODO this side detection isn't correct - need to look at firstSeg and check if
    //it potentially intersects before doing actual intersection test...

    IntersectionResult result;
    if (targetCentre.x < point.x && targetCentre.y < point.y)
    {
        //up left
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left + target.width, target.top + target.height));

        result.normal.y = 1.f;

        if (result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint); !result.intersects)
        {
            secondSegment =
                std::make_pair(sf::Vector2f(target.left + target.width, target.top + target.height), sf::Vector2f(target.left + target.width, target.top));
            result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint);

            result.normal.x = 1.f;
            result.normal.y = 0.f;
        }
    }
    else if (targetCentre.x > point.x && targetCentre.y < point.y)
    {
        //up right
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left + target.width, target.top + target.height));

        result.normal.y = 1.f;

        if (result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint); !result.intersects)
        {
            secondSegment =
                std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left, target.top));
            result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint);

            result.normal.x = -1.f;
            result.normal.y = 0.f;
        }
    }
    else if (targetCentre.x > point.x && targetCentre.y > point.y)
    {
        //down right
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top), sf::Vector2f(target.left + target.width, target.top));

        result.normal.y = -1.f;

        if (result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint); !result.intersects)
        {
            secondSegment =
                std::make_pair(sf::Vector2f(target.left, target.top + target.height), sf::Vector2f(target.left, target.top));
            result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint);

            result.normal.y = 0.f;
            result.normal.x = -1.f;
        }
    }
    else if (targetCentre.x < point.x && targetCentre.y > point.y)
    {
        //down left
        std::pair<sf::Vector2f, sf::Vector2f> secondSegment =
            std::make_pair(sf::Vector2f(target.left, target.top), sf::Vector2f(target.left + target.width, target.top));

        result.normal.y = -1.f;

        if (result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint); !result.intersects)
        {
            secondSegment =
                std::make_pair(sf::Vector2f(target.left + target.width, target.top + target.height), sf::Vector2f(target.left + target.width, target.top));
            result.intersects = intersects(firstSegment, secondSegment, result.intersectionPoint);

            result.normal.y = 0.f;
            result.normal.x = 1.f;
        }
    }

    return result;
};