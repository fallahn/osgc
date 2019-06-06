//root node representing the 3D scene//
#ifndef MESH_SCENE_H_
#define MESH_SCENE_H_

#include <Mesh/Node.h>
#include <Mesh/StaticMeshNode.h>
#include <Mesh/MeshResource.h>
#include <Mesh/Camera.h>
#include <Mesh/Light.h>

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>

#include <pugixml/pugixml.hpp>
#include <Game/ResourceManager.h>

namespace ml
{
	class MeshScene final 
		:	private Node,
			public sf::Drawable
	{
	public:
		explicit MeshScene(const sf::RenderWindow& rw);
		//creates a new light and returns a pointer to it
		LightPtr AddLight();
		//creates a new static mesh node and returns a pointer to it
		StaticMeshNode* AddStaticMesh(MeshBuffer& meshBuffer);
		//sets the view for the scene to follow
		void SetView(const sf::View& view);
		//updates the internal render texture
		void UpdateScene(float dt, const std::vector<sf::View>& views);
		//resizes gl viewport
		void Resize(); 
		//returns the texture containing rendered image
		const sf::Texture& GetTexture();

		//loads a scene from an XML file
		void LoadScene(const std::string& mapName);

		//draws the extracted self illumination buffer to given target
		void DrawSelfIllum(sf::RenderTarget& rt);

	private:

		std::unique_ptr<Camera> m_camera;
		Lights m_lights;
		MeshResource m_meshResource;
		Game::TextureResource  m_textureResource;

		sf::Sprite m_renderSprite;
		sf::RenderTexture m_renderTexture;
		const sf::RenderWindow& m_renderWindow;

		//buffer for rendering self illum parts to
		sf::Sprite m_illumSprite;
		sf::RenderTexture m_illumTexture;

		float m_cameraZ; //distance for camera from 2D scene
		const float m_sceneScale; //meshes are scaled by this so we can reduce cam distance

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
	typedef std::unique_ptr<MeshScene> ScenePtr;
}

#endif //MESH_SCENE_H_