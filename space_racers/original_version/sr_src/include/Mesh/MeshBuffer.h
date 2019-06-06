//wrapper class for vertex/colour/normal buffers which make up a single mesh//
//if you write a particular mesh format loader its result should be stored//
//in a mesh buffer object so that it can be drawn by the node graph//
#ifndef MESH_BUFFER_H_
#define MESH_BUFFER_H_

#include <SFML/System/Vector2.hpp>
#include <SFML/System/Vector3.hpp>
#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/Color.hpp>

#include <vector>
#include <memory>
#include <array>

#include <Mesh/GL/glew.h>

namespace ml
{
	class MeshBuffer : private sf::NonCopyable
	{
	public:
		//struct to represent a single vertex
		struct Vertex
		{
			sf::Vector3f position;
			sf::Vector3f normal; //NOT the same as a face normal
			sf::Vector2f texCoord;
			sf::Color colour;
		};
		//struct which represents a single triangle / face made of 3 vertices
		struct Face
		{
			std::array<sf::Uint32, 3> vertIndex; //indices of vertices in vertex array
			std::array<sf::Uint32, 3> normalIndex; //indices of vertex normals in normal array
			sf::Vector3f normal; //face normal
		};
		//enum for identifying data in a particular array
		enum BufferType
		{
			NONE,
			VERTEX_BUFFER,
			NORMAL_BUFFER,
			COLOUR_BUFFER,
			TEXTURE_BUFFER,
			INDEX_BUFFER
		};

		//typedefs for creating buffers
		typedef std::vector<Vertex> Vertices;
		typedef std::vector<Face> Faces;
		typedef std::vector<sf::Vector3f> Normals;

		//members
		MeshBuffer();
		~MeshBuffer();

		//sets the vertex array
		void SetVertices(const Vertices& vertices);
		//gets the face buffer array
		Faces& GetFaces();
		//gets the vertex buffer array
		Vertices& GetVertices();
		//gets the normal data array
		Normals& GetNormals();
		//allocates the gl buffers for mesh data
		//calc normals will internally calculate face normals
		//if the mesh loader / file format does not provide them
		//(embedded normals are preferable as calculating them here
		//will not take into account data such as smoothing groups)
		void Allocate(bool calcNormals = true);

		void Update();

		void Use(BufferType type);
	private:
		Vertices m_vertices; //vertex data array
		Normals m_normals; //normal data array
		Faces m_faces; //face data array

		//IDs of gl buffer objects
		GLuint m_vertexBufferId;
		GLuint m_normalBufferId;
		GLuint m_colourBufferId;
		GLuint m_textureBufferId;
		GLuint m_indexBufferId;

		//calculates face normals
		void m_CalcNormals();
	};

	typedef std::unique_ptr<MeshBuffer> BufferPtr;
	typedef std::vector<BufferPtr> BufferCollection;
}

#endif //MESH_BUFFER_H_