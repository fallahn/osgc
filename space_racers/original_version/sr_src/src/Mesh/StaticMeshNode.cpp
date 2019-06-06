///source for static mesh node class///
#include <Mesh/StaticMeshNode.h>
#include <Mesh/Shaders/Normal.h>
#include <Mesh/Shaders/Phong.h>
#include <Mesh/Shaders/SelfIllumination.h>

using namespace ml;

//ctor
StaticMeshNode::StaticMeshNode()
	:	m_meshBuffer(nullptr),
		m_texture	(nullptr),
		m_shader	(nullptr),
		m_sendMVMat	(false)
{

}

StaticMeshNode::StaticMeshNode(MeshBuffer& meshBuffer)
	:	m_meshBuffer(&meshBuffer),
		m_texture	(nullptr),
		m_shader	(nullptr),
		m_sendMVMat	(false)
{

}

//public
void StaticMeshNode::SetMeshBuffer(MeshBuffer& meshBuffer)
{
	m_meshBuffer = &meshBuffer;
}

void StaticMeshNode::SetTexture(sf::Texture& texture)
{
	m_texture = &texture;
	m_texture->setRepeated(true);
}

void StaticMeshNode::SetShader(ShaderProgram& shader)
{
	m_shader = &shader;
}

ShaderPtr StaticMeshNode::CreateShader(ShaderType type)
{
	switch(type)
	{
	case ShaderType::SelfIllum:
		m_illumShader.reset(new ShaderProgram());
		m_illumShader->loadFromMemory(sillumVert, sillumFrag);
		return m_illumShader;
	case ShaderType::NormalMap:
		m_intShader.reset(new ShaderProgram());
		m_intShader->loadFromMemory(normalVert, normalFrag);
		m_sendMVMat = true; //tell the mesh to send updated ModelView matrix to shader
		break;
	case ShaderType::PhongShade:
	default:
		m_intShader.reset(new ShaderProgram());
		m_intShader->loadFromMemory(phongVert, phongFrag);
		m_sendMVMat = false;
		break;
	}

	m_shader = m_intShader.get();
	return m_intShader;
}

//private
void StaticMeshNode::m_UpdateSelf(float dt)
{
	//check for scene node and update translation from it
	/*Game::SceneNode* sceneNode = m_GetSceneNode();
	if(sceneNode)
	{
		SetRotation(0.f, 0.f, sceneNode->getRotation());
		SetPosition(sceneNode->getPosition().x, sceneNode->getPosition().y, 0.f);
		SetScale(sf::Vector3f(sceneNode->getScale().x, sceneNode->getScale().y, 1.f));
	}*/


}

void StaticMeshNode::m_DrawSelf(RenderPass pass)
{
    if(!m_meshBuffer) return;

	//apply geometry
    glPushMatrix();
    m_ApplyTransform();
    m_ApplyMaterial();

	//bind texture if it exists
	if(m_texture)
	{
		glEnable(GL_TEXTURE_2D);
		sf::Texture::bind(m_texture);
	}

	//bind shader type based on pass param (quit if shader invalid in certain passes)
	if(pass == RenderPass::SelfIllum)
	{
		if(!m_illumShader) return; //TODO undo stuff
		else
		{
			ShaderProgram::bind(m_illumShader.get());
		}
	}
	else if(m_shader) ShaderProgram::bind(m_shader);
	
	//apply shader matrix for cube mapping
	if(m_sendMVMat)
	{
		std::array<float, 16> modelView;
		glGetFloatv(GL_MODELVIEW_MATRIX, &modelView[0]);
		m_shader->setParameter("modelView4x4", &modelView[0]);
	}

    glEnableClientState(GL_VERTEX_ARRAY);
    m_meshBuffer->Use(MeshBuffer::VERTEX_BUFFER);
    glVertexPointer(3, GL_FLOAT, 0, 0);

    glEnableClientState(GL_NORMAL_ARRAY);
    m_meshBuffer->Use(MeshBuffer::NORMAL_BUFFER);
    glNormalPointer(GL_FLOAT, 0, 0);

    glEnableClientState(GL_COLOR_ARRAY);
    m_meshBuffer->Use(MeshBuffer::COLOUR_BUFFER);
    glColorPointer(4, GL_UNSIGNED_BYTE, 0, 0);

    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    m_meshBuffer->Use(MeshBuffer::TEXTURE_BUFFER);
    glTexCoordPointer(2, GL_FLOAT, 0, 0);

    m_meshBuffer->Use(MeshBuffer::INDEX_BUFFER);
    glDrawElements(GL_TRIANGLES, m_meshBuffer->GetFaces().size() * 3, GL_UNSIGNED_INT, 0);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	ShaderProgram::bind(0);
	sf::Texture::bind(0);
	glDisable(GL_TEXTURE_2D);

    glPopMatrix();
}

