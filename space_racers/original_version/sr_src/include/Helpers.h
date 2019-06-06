//header file containing static utility methods//
#ifndef HELPERS_H_
#define HELPERS_H_

#include <math.h>
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Text.hpp>
#include <string>
#include <sstream>

//helper functions
namespace Helpers
{
	namespace Random
	{
		const float Float(float begin, float end);

		const int Int(int begin, int end);
	};
	
	namespace Vectors
	{
		//calculates dot product of 2 vectors
		static float Dot(const sf::Vector2f& lv, const sf::Vector2f& rv)
		{
			return lv.x * rv.x + lv.y * rv.y;
		}		
		
		//Returns a given vector with its length normalized to 1
		static sf::Vector2f Normalize(sf::Vector2f source)
		{
			float length = std::sqrt(Dot(source, source));
			if (length != 0) source /= length;

			return source;
		}

		//Returns angle in degrees of a given vector where 0 is horizontal
		static float GetAngle(const sf::Vector2f& source)
		{
			return atan2(source.y , source.x) * 180.f / 3.14159265f;
		}

		//returns length squared
		static float GetLengthSquared(const sf::Vector2f& source)
		{
			return Dot(source, source);
		}

		//Returns length of a given vector
		static float GetLength(const sf::Vector2f& source)
		{
			return std::sqrt(GetLengthSquared(source));
		}

		//Returns Normalized vector from given angle in degrees where 0 is horizontal
		static sf::Vector2f GetVectorFromAngle(float angle)
		{
			if(angle < 0) angle += 360; //make sure angle is always positive value
			switch((int)angle)
			{
			case 0:
			case 360:
				return sf::Vector2f(1.f, 0.f);
			case 90:
			//case -270:
				return sf::Vector2f(0.f, 1.f);
			case 180:
			//case -180:
				return sf::Vector2f(-1.f, 0.f);
			case 270:
			//case -90:
				return sf::Vector2f(0.f, -1.f);
				break;
			default:
				angle = (float)(angle * (3.14159265 / 180));
				return sf::Vector2f(cos(angle), sin(angle));
			}
		}

		//calculates cross product of 2 vectors
		static float Cross(const sf::Vector2f& lv, const sf::Vector2f& rv)
		{
			return lv.x * rv.y - lv.y * rv.x;
		}
		static float Cross(const sf::Vector2f& a, const sf::Vector2f& b, const sf::Vector2f& c)
		{
			sf::Vector2f BA = a - b;
			sf::Vector2f BC = c - b;
			return (BA.x * BC.y - BA.y * BC.x);
		}

		//project onto dest
		static sf::Vector2f Project(sf::Vector2f source, sf::Vector2f dest)
		{
			return dest * Dot(source, dest);
		}
		//project onto dest with signed magnitude
		static sf::Vector2f Project(sf::Vector2f source, sf::Vector2f dest, float& mag)
		{
			float sourceDotDest = Dot(source, dest);
			mag = sourceDotDest;
			return dest * sourceDotDest;
		}

		//perpendicular vector of given length
		static sf::Vector2f GetPerpendicular(const sf::Vector2f& source, float length = 1.f)
		{			
			return Normalize(sf::Vector2f(source.y, -source.x)) * length + source;
		}

		//reflection of a vector about a Normal
		static sf::Vector2f Reflect(const sf::Vector2f& v, const sf::Vector2f& n)
		{
			return -2 * Dot(v, n) * n + v;
		}
	};

	namespace Rectangles
	{
		static sf::Vector2f GetIntersectionDepth(const sf::FloatRect& rectA, const sf::FloatRect& rectB)
        {
            // Calculate half sizes.
            float halfWidthA = rectA.width / 2.f;
            float halfHeightA = rectA.height / 2.f;
            float halfWidthB = rectB.width / 2.f;
            float halfHeightB = rectB.height / 2.f;

            // Calculate centers.
			sf::Vector2f centerA = sf::Vector2f(rectA.left + halfWidthA, rectA.top + halfHeightA);
			sf::Vector2f centerB = sf::Vector2f(rectB.left + halfWidthB, rectB.top + halfHeightB);

            // Calculate current and minimum-non-intersecting distances between centers.
            float distanceX = centerA.x - centerB.x;
            float distanceY = centerA.y - centerB.y;
            float minDistanceX = halfWidthA + halfWidthB;
            float minDistanceY = halfHeightA + halfHeightB;

            // If we are not intersecting at all, return (0, 0).
            if (abs(distanceX) >= minDistanceX || abs(distanceY) >= minDistanceY)
				return sf::Vector2f();

            // Calculate and return intersection depths.
            float depthX = distanceX > 0 ? minDistanceX - distanceX : -minDistanceX - distanceX;
            float depthY = distanceY > 0 ? minDistanceY - distanceY : -minDistanceY - distanceY;
			return sf::Vector2f(depthX, depthY);
        }

		static sf::FloatRect GetViewRect(const sf::View& view)
		{
			return sf::FloatRect(view.getCenter().x - (view.getSize().x / 2.f),
								view.getCenter().y - (view.getSize().y / 2.f),
								view.getSize().x,
								view.getSize().y);
		}
	};

	namespace Math
	{
		static float Clamp(float x, float a, float b)
		{
			return x < a ? a : (x > b ? b : x);
		}

		static float Round(float val)
		{
			return floor(val + 0.5f);
		}
	};

	namespace String
	{
        template<typename T>
        static T GetFromString(const std::string& strValue, const T& defaultValue)
        {
            std::istringstream iss(strValue);
            T back;
            if (iss >> back)
                return back;
            return defaultValue;
        }
	};

	namespace Data
	{

	};

	namespace Position
	{
		static void CentreOrigin(sf::Sprite& sprite)
		{
			sf::FloatRect bounds = sprite.getLocalBounds();
			sprite.setOrigin(std::floor(bounds.width / 2.f), std::floor(bounds.height / 2.f));
		}

		static void CentreOrigin(sf::Text& text)
		{
			sf::FloatRect bounds = text.getLocalBounds();
			text.setOrigin(std::floor(bounds.width / 2.f), std::floor(bounds.height / 2.f));
		}
	}
};

#endif