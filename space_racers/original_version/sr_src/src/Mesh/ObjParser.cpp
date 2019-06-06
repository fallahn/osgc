///implements the obj file parser///
#include <Mesh/ObjParser.h>
#include <Mesh/MeshHelpers.h>
#include <Helpers.h>

#include <iostream>
#include <fstream>
#include <cassert>

using namespace ml;

//public
BufferPtr ObjParser::LoadFromFile(const std::string& path)
{
    std::fstream file(path.c_str());
    if (file.fail())
    {
        std::cerr << "Unable to load mesh file \"" << path << "\" " << std::endl;
        return nullptr;
    }

	//TODO exception handling when obj file not valid
	std::vector<sf::Vector2f> texPos;
	std::vector<sf::Vector3f> vertPos;
	std::vector<sf::Vector3f> normals;
	std::vector<ObjParser::Face> faces;

    std::string line; //"v " is position, "vt" is tex coord, "vn" is normal vector
    while (std::getline(file, line))
    {
        switch (line[0])
        {
            case 'v' :
            {
                    
				switch (line[1])
                {
                    case ' ': //vertex pos
                        vertPos.push_back(m_GetVectorFromLine(line));
						break;
					case 'n':
						//normal data
						normals.push_back(m_GetVectorFromLine(line));
						break;
					case 't':
						//uv data
						texPos.push_back(m_GetTexCoordFromLine(line));
						break;
                };
            }
            break;

			//even though this is in the same switch block we assume all the vert
			//and texture coords have been parsed out of the file
            case 'f' :
                faces.push_back(m_GetFaceFromLine(line));
				break;
        };
    }

	//TODO create multiple buffers for each texture used

	BufferPtr meshBuffer(new MeshBuffer());
    MeshBuffer::Vertices& bufferVerts = meshBuffer->GetVertices();
    MeshBuffer::Faces& bufferFaces = meshBuffer->GetFaces();

	//parse each face and create a new MeshBuffer triangle with unique verts
	for(auto& f : faces)
	{
		MeshBuffer::Face newFace;
		for(auto i = 0u; i < 3u; i++)
		{
			MeshBuffer::Vertex newVert;
			newVert.position = vertPos[f.vIndex[i]];
			if(f.fIndex[i] > -1) newVert.texCoord = texPos[f.fIndex[i]];
			if(f.nIndex[i] > -1)
			{
				newFace.normalIndex[i] = f.nIndex[i];
				newVert.normal = normals[f.nIndex[i]];
			}
			bufferVerts.push_back(newVert);
			newFace.vertIndex[i] = bufferVerts.size() - 1u;
		}
		bufferFaces.push_back(newFace);
	}

	bool calcNormals = normals.size() == 0u;
    meshBuffer->Allocate(calcNormals);
    return std::move(meshBuffer);
}

//private
sf::Vector3f ObjParser::m_GetVectorFromLine(const std::string& line)
{
	std::vector<std::string> point = m_Split(line, ' ');
	assert(point.size() >= 4);

	return sf::Vector3f(Helpers::String::GetFromString<float>(point[1], 0.f),
						Helpers::String::GetFromString<float>(point[2], 0.f),
						Helpers::String::GetFromString<float>(point[3], 0.f));
}

sf::Vector2f ObjParser::m_GetTexCoordFromLine(const std::string& line)
{
	std::vector<std::string> coord = m_Split(line, ' ');
	assert(coord.size() >= 3);

	return sf::Vector2f(Helpers::String::GetFromString<float>(coord[1], 0.f),
						1.f - Helpers::String::GetFromString<float>(coord[2], 0.f));
}

ObjParser::Face ObjParser::m_GetFaceFromLine(const std::string& line)
{
    //Get all elements as string
	ObjParser::Face face;
    sf::Uint8 i = 0;
	std::vector<std::string> data = m_Split(line, ' ');
	assert(data.size() >= 4);
	for (auto& str : data)
    {
        if (i != 0)
        {
			std::vector<std::string> values = m_Split(str, '/', true);			
			while(values.size() < 3) values.push_back("0");
			
			//vertex ID
            face.vIndex[i - 1] = Helpers::String::GetFromString<sf::Uint32>(values[0], 0) - 1u;

			//texCoord ID
			face.fIndex[i - 1] = Helpers::String::GetFromString<sf::Uint32>(values[1], 0) - 1u;

			//normal coord ID
            face.nIndex[i - 1] = Helpers::String::GetFromString<sf::Uint32>(values[2], 0) - 1u;
        }
        i++;

		if(i > 4)
		{
			std::cerr << "found more than 3 vertices per face - only triangle format files are supported." << std::endl;
			return face;
		}
    }
    return face;
}

std::vector<std::string>& ObjParser::m_Split(const std::string& str, char delim, std::vector<std::string>& output, bool keepEmpty)
{
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, delim))
	{
		if (!token.empty() ||
			(token.empty() && keepEmpty))
		{
			output.push_back(token);
		}
	}
	return output;
}

std::vector<std::string> ObjParser::m_Split(const std::string& str, char delim, bool keepEmpty)
{
	std::vector<std::string> output;
	m_Split(str, delim, output, keepEmpty);
	return output;
}