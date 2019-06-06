///source for mesh buffer class///
#include <Mesh/MeshBuffer.h>
#include <Mesh/MeshHelpers.h>

using namespace ml;

//ctor / dtor
MeshBuffer::MeshBuffer()
{
	//create gl buffers for mesh data
	glGenBuffers(1, &m_vertexBufferId);
	glGenBuffers(1, &m_normalBufferId);
	glGenBuffers(1, &m_colourBufferId);
	glGenBuffers(1, &m_textureBufferId);
	glGenBuffers(1, &m_indexBufferId);
}

MeshBuffer::~MeshBuffer()
{
	//make sure to delete any gl buffers
	glDeleteBuffers(1, &m_vertexBufferId);
	glDeleteBuffers(1, &m_normalBufferId);
	glDeleteBuffers(1, &m_colourBufferId);
	glDeleteBuffers(1, &m_textureBufferId);
	glDeleteBuffers(1, &m_indexBufferId);	
}


//public
void MeshBuffer::SetVertices(const Vertices& vertices)
{
	m_vertices = vertices;
}

MeshBuffer::Faces& MeshBuffer::GetFaces()
{
	return m_faces;
}

MeshBuffer::Vertices& MeshBuffer::GetVertices()
{
	return m_vertices;
}

MeshBuffer::Normals& MeshBuffer::GetNormals()
{
	return m_normals;
}

void MeshBuffer::Allocate(bool calcNormals)
{
    //allocates the member data to the gl buffer objects
	sf::Uint32 vertexSize = m_vertices.size(), indexSize = m_faces.size() * 3;

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * 3 * sizeof(float), 0, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_normalBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * 3 * sizeof(float), 0, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_colourBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * 4 * sizeof(sf::Uint8), 0, GL_STREAM_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, m_textureBufferId);
    glBufferData(GL_ARRAY_BUFFER, vertexSize * 2 * sizeof(float), 0, GL_STREAM_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexSize * sizeof(sf::Uint32), 0, GL_STREAM_DRAW);

    if(calcNormals) m_CalcNormals();
    Update();
}

void MeshBuffer::Update()
{
    sf::Uint32 vertexSize = m_vertices.size(), indexSize = m_faces.size() * 3;
    sf::Uint32 v = 0, n = 0, t = 0, c = 0, i = 0;

    std::unique_ptr<float[]> vertices(new float[vertexSize * 3]);
	std::unique_ptr<float[]> normals(new float[vertexSize * 3]);
    std::unique_ptr<float[]> texCoords(new float[vertexSize * 2]);
	std::unique_ptr<sf::Uint8[]> colors(new sf::Uint8[vertexSize * 4]);
    std::unique_ptr<sf::Uint32[]> indices(new sf::Uint32[indexSize]);

    for(const auto& vertex : m_vertices)
    {
        vertices[v + 0] = vertex.position.x;
        vertices[v + 1] = vertex.position.y;
        vertices[v + 2] = vertex.position.z;
        v += 3;

        normals[n + 0] = vertex.normal.x;
        normals[n + 1] = vertex.normal.y;
        normals[n + 2] = vertex.normal.z;
        n += 3;

        colors[c + 0] = vertex.colour.r;
        colors[c + 1] = vertex.colour.g;
        colors[c + 2] = vertex.colour.b;
        colors[c + 3] = vertex.colour.a;
        c += 4;

        texCoords[t + 0] = vertex.texCoord.x;
        texCoords[t + 1] = vertex.texCoord.y;
        t += 2;
    }

    for(const auto& face : m_faces)
    {
        indices[i + 0] = face.vertIndex[0];

        indices[i + 1] = face.vertIndex[1];

        indices[i + 2] = face.vertIndex[2];

        i += 3;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize * 3 * sizeof(float), vertices.get());

    glBindBuffer(GL_ARRAY_BUFFER, m_normalBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize * 3 * sizeof(float), normals.get());

    glBindBuffer(GL_ARRAY_BUFFER, m_colourBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize * 4 * sizeof(sf::Uint8), colors.get());

    glBindBuffer(GL_ARRAY_BUFFER, m_textureBufferId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, vertexSize * 2 * sizeof(float), texCoords.get());

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexSize * sizeof(sf::Uint32), indices.get());
}

void MeshBuffer::Use(MeshBuffer::BufferType type)
{
    switch(type)
	{
	case VERTEX_BUFFER:
        glBindBuffer(GL_ARRAY_BUFFER, m_vertexBufferId);
		return;
	case COLOUR_BUFFER:
        glBindBuffer(GL_ARRAY_BUFFER, m_colourBufferId);
		return;
	case NORMAL_BUFFER:
        glBindBuffer(GL_ARRAY_BUFFER, m_normalBufferId);
		return;
	case TEXTURE_BUFFER:
        glBindBuffer(GL_ARRAY_BUFFER, m_textureBufferId);
		return;
	case INDEX_BUFFER:
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBufferId);
		return;
	default:
        glBindBuffer(GL_ARRAY_BUFFER, 0);
		return;
	}
}

//private
void MeshBuffer::m_CalcNormals()
{
    for(auto& f : m_faces)
    {
        sf::Vector3f v[3] = { m_vertices[f.vertIndex[0]].position, m_vertices[f.vertIndex[1]].position, m_vertices[f.vertIndex[2]].position };
		f.normal = Helpers::Vectors::Cross(v[1] - v[0], v[2] - v[0]);

        for (sf::Uint8 i = 0; i < 3; ++i)
        {
            sf::Vector3f a = v[(i + 1) % 3] - v[i];
            sf::Vector3f b = v[(i + 2) % 3] - v[i];
			float weight = std::acos(Helpers::Vectors::Dot(a, b) / (Helpers::Vectors::GetLength(a) * Helpers::Vectors::GetLength(b)));
            m_vertices[f.vertIndex[i]].normal += weight * f.normal;
        }
    }

	for(auto& v : m_vertices)
    {
		v.normal = Helpers::Vectors::Normalize(v.normal);
    }
}