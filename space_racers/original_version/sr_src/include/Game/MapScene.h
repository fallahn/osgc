////class for creating a 3D scene for an associated map///
#ifndef MAP_SCENE_H_
#define MAP_SCENE_H_

#include <Game/Common.h>
#include <Game/SharedData.h>
#include <memory>

namespace Game
{
	class MapScene : public sf::Drawable, private sf::NonCopyable
	{
	public:
		MapScene(sf::RenderWindow& renderWindow, TextureResource& textures);

		//builds the scene node for given map name
		bool CreateScene(const std::string& mapName);
		//updates the camera position
		void UpdateCamera(float x, float y);
		//resizes the viewport to the current window
		void Resize();
		//updates the render texture with the 3D scene
		void Update();
		//returns a reference to the render buffer texture
		const sf::Texture& GetBufferTexture() const{return m_renderTexture.getTexture();};
		//void Reset(){};
		sf::View GetView() const{return m_renderTexture.getView();};
	private:
		std::vector< std::unique_ptr<sf3d::MeshNode> > m_sceneNodes;
		sf3d::OBJMesh m_mesh;
		sf3d::Scene m_scene;
		sf3d::Camera m_camera;
		sf3d::Light m_light;

		sf::RenderTexture m_renderTexture;
		sf::Sprite m_sprite;
		sf::Shader m_postShader;

		float m_zPos; //distance to the camera
		const float m_scale; //models need to be scaled slightly
		sf::RenderWindow& m_renderWindow; //reference to render window so we can update rendere size
		TextureResource& m_textureResource;

		void draw(sf::RenderTarget& rt, sf::RenderStates states) const;
	};
}
#endif //MAP_SCENE_H_