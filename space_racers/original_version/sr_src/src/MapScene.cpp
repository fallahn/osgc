///source for map scene class

#include <Game/MapScene.h>
#include <sf3d/MaterialFile.hpp>
#include <sf3d/Shaders/FxAA.h>
#include <fstream>

using namespace Game;

//ctor
MapScene::MapScene(sf::RenderWindow& renderWindow, TextureResource& textures)
	: m_renderWindow	(renderWindow),
	m_textureResource	(textures),
	m_zPos				(-77.14f),
	m_scale				(0.1f)
{
	//m_postShader.loadFromMemory(fxaaVert, fxaaFrag);
}

//public
bool MapScene::CreateScene(const std::string& mapName)
{
	//update render buffer to window size
	Resize(); //if we do this anywhere else it breaks normal mapping :S
	
	//clear existing data
	m_sceneNodes.clear();

	//parse model list from map name
	std::vector<std::string> meshList;
	std::ifstream file("assets/maps/" + mapName + ".mlst", std::ifstream::in);
	if(file.fail())
	{
		std::cerr << "Warning: failed to open mesh list file " << std::endl;
		return false;
	}

	std::string line;
	while(std::getline(file, line))	
		meshList.push_back(line);

	//create node for each model listed using material files
	for(auto& mesh : meshList)
	{
		std::unique_ptr<sf3d::MeshNode> node(new sf3d::MeshNode());

		if(!m_mesh.loadFromFile("assets/meshes/" + mesh + ".obj", node->getMeshBuffer()))
		{
			std::cerr << "Skipping create of " << mesh << " node" << std::endl;
			continue;
		}
		
		sf3d::MaterialFile mFile;
		if(!mFile.loadFromFile("assets/materials/" + mesh + ".sf3d"))
			std::cerr << "Missing or corrupt material file for: " << mesh << std::endl;
		else
		{
			//continue to create node
			node->setTexture(m_textureResource.Get("assets/textures/materials/" + mFile.diffuseMap()));
			node->setNormalMap(m_textureResource.Get("assets/textures/materials/" + mFile.normalMap()));
			node->setReflectionMap(m_textureResource.Get("assets/textures/environment/nebula.jpg"));

			//TODO parse material properties

			//std::cout <<"Loaded some shit, or some shit" << std::endl;
		}

		//scale node
		node->setScale(sf::Vector3f(m_scale, m_scale, m_scale));
		m_sceneNodes.push_back(std::move(node));
	}

	//add nodes to new scene
	m_scene.clearChildren();
	for(auto&& node : m_sceneNodes)
		m_scene.add(node.get());


	//TODO set light position from map
	m_light.setPosition(sf::Vector3f(256.f, 204.8f, 629.4f));
	m_scene.addLight(&m_light);

	//add camera to scene
	m_scene.setCamera(&m_camera);
	
	return true;
}

void MapScene::UpdateCamera(float x, float y)
{
	m_camera.setPosition(-x * m_scale, y * m_scale, m_zPos);

	//if(sf::Keyboard::isKeyPressed(sf::Keyboard::Home))
	//{
	//	//m_camera.move(0.f, -0.8f, 0.f);
	//	m_zPos--;
	//	std::cout << m_zPos << std::endl;
	//}
	//else if(sf::Keyboard::isKeyPressed(sf::Keyboard::End))
	//{
	//	m_zPos++;
	//	std::cout << m_zPos << std::endl;
	//}
}

void MapScene::Resize()
{
    sf::Vector2ui winSize = m_renderWindow.getSize();
	
	m_renderTexture.create(winSize.x, winSize.y, true);
	m_sprite.setTexture(m_renderTexture.getTexture(), true);
	m_renderTexture.setActive(true);
	glViewport(0, 0, winSize.x, winSize.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	//set camera position
	m_camera.setAspectRatio(static_cast<float>(m_renderWindow.getSize().x) / m_renderWindow.getSize().y);
	const float angle = std::tan(m_camera.getFOV() / 2.f * 0.0174532925f);
	m_zPos = (static_cast<float>(m_renderWindow.getView().getSize().y) / 2.f) / angle;	
	m_zPos *= -m_scale;

	//update post shader
	//m_postShader.setParameter("rt_w", static_cast<float>(winSize.x));
	//m_postShader.setParameter("rt_h", static_cast<float>(winSize.y));
}

void MapScene::Update()
{
	if(m_sceneNodes.size() == 0) return;
	m_renderTexture.setActive(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    m_scene.render(sf3d::Node::PASS_SOLID);
	
	m_renderTexture.display();
}

//private
void MapScene::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	//states.shader = &m_postShader;
	rt.draw(m_sprite, states);
}