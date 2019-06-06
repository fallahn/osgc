///implements mesh resource class///
#include <Mesh/MeshResource.h>
#include <Mesh/ObjParser.h>
#include <assert.h>

using namespace ml;

//ctor
MeshResource::MeshResource(){}

//public
MeshBuffer& MeshResource::Get(const std::string& path, ml::FileType type)
{
	assert(!path.empty());

	//check if mesh already loaded
	auto m = m_meshes.find(path);
	if(m != m_meshes.end())
		return *m->second;

	//else attempt to load mesh via type parser
	BufferPtr mb;
	switch(type)
	{
	case FileType::Obj:
		mb = ObjParser::LoadFromFile(path);
		assert(mb != nullptr);
		m_meshes[path] = std::move(mb);
		break;
	}
		return *m_meshes[path];
}

//private