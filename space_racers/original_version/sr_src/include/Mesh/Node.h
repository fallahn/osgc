///base class for 3D scene nodes. Analogous to the SceneNode class but dedicated
//to 3D scenes. These nodes can be tied to SceneNoes so that they may update each other.
//As 3D nodes are meant ot be pure visual it is better to update a node from its
//SceneNode partner, and not the other way around.
#ifndef NODE_H_
#define NODE_H_

#include <SFML/System/Vector3.hpp>
#include <SFML/System/String.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

#include <Mesh/GL/glew.h>
#include <Mesh/Light.h>
#include <Mesh/Material.h>
#include <Mesh/MeshHelpers.h>
#include <Mesh/ShaderProgram.h>

//#include <Game/SceneNode.h>

#include <vector>
#include <memory>

namespace ml
{
	class Node;
	typedef std::unique_ptr<Node> NodePtr;
	typedef std::vector<NodePtr> Nodes;

	class Node : private sf::NonCopyable
	{
	public:
		enum class RenderPass
		{
			Final,
			SelfIllum
		};

		Node();
		virtual ~Node();

		//sets 3d world position
		void SetPosition(const sf::Vector3f& position);
		void SetPosition(float x, float y, float z);
		//returns position in 3d world
		const sf::Vector3f& GetPosition() const;
		//moves node - equivalent of position + move parameter
		void Move(const sf::Vector3f& move);
		void Move(float x, float y, float z);
		//sets the node rotation
		void SetRotation(const sf::Vector3f& rotation);
		void SetRotation(float x, float y, float z);
		//returns the nodes current rotation
		const sf::Vector3f& GetRotation() const;
		//rotates the node - equivalent of current rotation + rotate parameter
		void Rotate(const sf::Vector3f& rotate);
		void Rotate(float x, float y, float z);
		//sets the nodes scale
		void SetScale(const sf::Vector3f& scale);
		//returns the nodes current scale
		const sf::Vector3f& GetScale() const;
		//scales the node - equvalent of current scale + scale parameter
		void Scale(const sf::Vector3f& scale);
		//applies a scale based on scene scale when matching 3D with SFML 2D units
		void SetSceneScale(float sceneScale);
		//allows attaching to a 2D scenenode
		//void SetSceneNode(Game::SceneNode& node);
		//adds a child node
		void AddChild(NodePtr node);
		//removes a child node
		NodePtr RemoveChild(const Node& node);
		//applies a material to a node
		void SetMaterial(const Material& material);
		//gets the name of the current material
		const Material& GetMaterial() const;
		//applies lighting to node data
		void ApplyLighting(Lights& lights);

		void Update(float dt);
		void Draw(RenderPass pass = RenderPass::Final);

    protected:


        void m_ApplyTransform();
        void m_ApplyMaterial();

		virtual void m_UpdateSelf(float dt);
		virtual void m_DrawSelf(RenderPass pass = RenderPass::Final);

		Material& m_Material(){return m_material;}
		Nodes& m_GetChildren(){return m_children;}
		//Game::SceneNode* m_GetSceneNode()const;

	private:
        //world coordinates
        sf::Vector3f m_position, m_rotation, m_scale;

        //Parent / children
        Nodes m_children;
        Node* m_parent;
		//Game::SceneNode* m_sceneNode; //attach a mesh to a scene node to recieve 2D transform updates

        //Materials
        Material m_material;

		void m_ClampRotation();
	};
}

#endif //NODE_H_