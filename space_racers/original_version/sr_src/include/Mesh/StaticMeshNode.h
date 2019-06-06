///a graph node with a static mesh buffer. If you want an animated mesh
///created from multiple frames / mesh buffers (such as MD2 format) use
///DynamicMeshNode.
#ifndef STATIC_MESH_H_
#define STATIC_MESH_H_

#include <SFML/Graphics/Texture.hpp>

#include <Mesh/Node.h>
#include <Mesh/MeshBuffer.h>
#include <Mesh/ShaderProgram.h>

//#include <Game/SceneNode.h>

#include <memory>

namespace ml
{
	enum class ShaderType
	{
		NormalMap,
		PhongShade,
		SelfIllum
	};
	
	class StaticMeshNode : public Node
	{
	public:
		StaticMeshNode();
		StaticMeshNode(MeshBuffer& buffer);

		void SetMeshBuffer(MeshBuffer& meshBuffer);
		void SetTexture(sf::Texture& texture);
		void SetShader(ShaderProgram& shader);

		//creates a new shader of the given type, returns a pointer to it and sets it active on the node
		ShaderPtr CreateShader(ShaderType type);
	private:
		MeshBuffer* m_meshBuffer;
		sf::Texture* m_texture; //base texture for mesh. Assumes other textures such as normal maps are handled by shader
		ShaderProgram* m_shader; //currently active shader - separate from intShader so can be set to an external program

		//internal shaders
		ShaderPtr m_intShader; 
		ShaderPtr m_illumShader; //create this if we want the mesh drawn on self illum buffer
		bool m_sendMVMat; //enable sending ModelView matrix to shaders which need it

		void m_UpdateSelf(float dt);
		void m_DrawSelf(RenderPass pass = RenderPass::Final);
	};
}

#endif //STATIC_MESH_H_