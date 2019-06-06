///helper functions for 3D vectors///
#ifndef MESH_HELPERS_H_
#define MESH_HELPERS_H_

#include <SFML/System/Vector3.hpp>
#include <cmath>
#include <Mesh/GL/glew.h>

namespace Helpers
{
	const float pi = 3.141592654f;

	namespace Vectors
	{
        static sf::Vector3f Cross(const sf::Vector3f& v1, const sf::Vector3f& v2)
        {
            sf::Vector3f vect;
            vect.x = v1.y * v2.z - v1.z * v2.y;
            vect.y = v1.z * v2.x - v1.x * v2.z;
            vect.z = v1.x * v2.y - v1.y * v2.x;
            return vect;
        }

        static float GetLength(const sf::Vector3f& vect)
        {
            return std::sqrt(vect.x * vect.x + vect.y * vect.y + vect.z * vect.z);
        }

        static float GetLengthSquared(const sf::Vector3f& vect)
        {
            return vect.x * vect.x + vect.y * vect.y + vect.z * vect.z;
        }

        static float Dot(const sf::Vector3f& v1, const sf::Vector3f& v2)
        {
            return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
        }

        static sf::Vector3f Normalize(const sf::Vector3f vect)
        {
            float length = GetLength(vect);
            sf::Vector3f v;
            v.x = vect.x / length;
            v.y = vect.y / length;
            v.z = vect.z / length;
            return v;
        }
	}
}


#endif //MESH_HELPERS_H_