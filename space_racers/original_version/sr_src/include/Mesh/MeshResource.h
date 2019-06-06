///provides a resource manager for the uniform MeshBuffer class
///with the ability to load mesh data from various file types

#ifndef MESH_RESOURCE_H_
#define MESH_RESOURCE_H_

#include <Mesh/MeshBuffer.h>
#include <map>
#include <string>

namespace ml
{
	enum class FileType
	{
		Obj,
		MD2
		//3ds etc
	};

	class MeshResource : private sf::NonCopyable
	{
	public:
		MeshResource();
		MeshBuffer& Get(const std::string& path, FileType type);

	private:
		std::map<std::string, BufferPtr> m_meshes;
	};
}

#endif