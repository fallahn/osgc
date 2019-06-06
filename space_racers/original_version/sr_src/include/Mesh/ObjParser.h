////parses mesh data from obj files into a mesh buffer////
#ifndef OBJ_FILE_H_
#define OBJ_FILE_H_

#include <SFML/System/NonCopyable.hpp>
#include <Mesh/MeshBuffer.h>
#include <array>

namespace ml
{
	class ObjParser final : private sf::NonCopyable
	{
	public:
		static BufferPtr LoadFromFile(const std::string& path);

	private:
		struct Face
		{
			Face()
			{
				for(auto i = 0u; i < 3u; i++)
				{
					vIndex[i] = -1;
					fIndex[i] = -1;
					nIndex[i] = -1;
				}
			}
			std::array<sf::Int16, 3> vIndex, fIndex, nIndex;
		};
		//gets vertex position
        static sf::Vector3f m_GetVectorFromLine(const std::string& line);
		//gets texCoord
		static sf::Vector2f m_GetTexCoordFromLine(const std::string& line);
		//gets face
        static Face m_GetFaceFromLine(const std::string& line);
		//utility funcs for splitting string data
		static std::vector<std::string>& m_Split(const std::string& str, char delim, std::vector<std::string>& output, bool keepEmpty = false);
		static std::vector<std::string> m_Split(const std::string& str, char delim, bool keepEmpty = false);
	};
}

#endif //OBJ_FILE_H_