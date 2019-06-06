//implements drawable root node of a mesh scene graph///
#include <Mesh/MeshScene.h>
#include <iostream>
#include <assert.h>

using namespace ml;

namespace
{
	//config file directories here
	std::string meshPath = "assets/meshes/"; //remember trailing /
	std::string mapPath = "assets/maps/";
	std::string texturePath = "assets/textures/materials/";
	std::string materialPath = "assets/materials/";
	std::string reflectMapPath = "assets/textures/environment/nebula_env.jpg";
}

//ctor
MeshScene::MeshScene(const sf::RenderWindow& rw)
	:	m_camera	(new Camera),
	m_renderWindow	(rw),
	m_cameraZ		(0.f),
	m_sceneScale	(0.1f)
{
	Resize();
}

//public
LightPtr MeshScene::AddLight()
{
	LightPtr l(new Light);
	m_lights.push_back(l);
	return l;
}

StaticMeshNode* MeshScene::AddStaticMesh(MeshBuffer& meshBuffer)
{
	StaticMeshNode* node = new StaticMeshNode(meshBuffer);
	AddChild(NodePtr(node));
	return node;
}

void MeshScene::SetView(const sf::View& view)
{
	m_camera->SetAspectRatio(view.getSize().x / view.getSize().y);	
	const float angle = std::tan(m_camera->GetFOV() / 2.f * 0.0174532925f);
	m_cameraZ = (static_cast<float>(view.getSize().y) / 2.f) / angle;	
	m_cameraZ *= -m_sceneScale;

	m_camera->SetPosition(-view.getCenter().x * m_sceneScale,
						view.getCenter().y * m_sceneScale,
						m_cameraZ);

	//update view ports so we can draw split screen
	sf::Vector2u winSize = m_renderWindow.getSize();
	GLuint x = static_cast<GLuint>(view.getViewport().left * static_cast<float>(winSize.x));
	GLuint y = static_cast<GLuint>((1.f - view.getViewport().top) * static_cast<float>(winSize.y));
	GLuint w = static_cast<GLuint>(view.getViewport().width * static_cast<float>(winSize.x));
	GLuint h = static_cast<GLuint>(view.getViewport().height * static_cast<float>(winSize.y));

	//invert position
	y -= h;

	//m_illumTexture.setActive(true);
	glViewport(x, y, w, h);

	//m_renderTexture.setActive(true);
	//glViewport(x, y, w, h);

	//std::cerr << x << " " << y << " " << w << " " << h << std::endl;
}

void MeshScene::UpdateScene(float dt, const std::vector<sf::View>& views)
{	
	Update(dt); //updates self and all children

	//render any node with self illum to own buffer
	Nodes& children = m_GetChildren();

	m_illumTexture.setActive(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//loop through all views
	for(const auto& v : views)
	{
		SetView(v);
		m_camera->Draw();
		for(auto&& c : children)
			c->Draw(RenderPass::SelfIllum);
	}

	m_illumTexture.display();

	//update render texture
	m_renderTexture.setActive(true);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	//glEnable(GL_MULTISAMPLE_ARB);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);

	for(const auto& v : views)
	{
		SetView(v);
		m_camera->Draw();
		for(auto&& c : children)
		{		
			c->ApplyLighting(m_lights);
			c->Draw();
		}
	}

	glDisable(GL_BLEND);
	m_renderTexture.display();
}

void MeshScene::Resize()
{
    sf::Vector2u winSize = m_renderWindow.getSize();

	//update self illum buffer
	m_illumTexture.create(winSize.x, winSize.y, true);
	m_illumSprite.setTexture(m_illumTexture.getTexture(), true);
	m_illumTexture.setActive(true);
	glViewport(0, 0, winSize.x, winSize.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	
	
	//and update main scene buffer
	m_renderTexture.create(winSize.x, winSize.y, true);
	m_renderSprite.setTexture(m_renderTexture.getTexture(), true);
	m_renderTexture.setActive(true);
	glViewport(0, 0, winSize.x, winSize.y);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	//set camera position
	m_camera->SetAspectRatio(static_cast<float>(winSize.x) / winSize.y);
	const float angle = std::tan(m_camera->GetFOV() / 2.f * 0.0174532925f);
	m_cameraZ = (static_cast<float>(m_renderWindow.getView().getSize().y) / 2.f) / angle;	
	m_cameraZ *= -m_sceneScale;
}

const sf::Texture& MeshScene::GetTexture()
{
	return m_renderTexture.getTexture();
}

void MeshScene::LoadScene(const std::string& mapName)
{
	m_GetChildren().clear(); //remove existing scene data

	//attempt to open file
	std::string path = mapPath + mapName + ".sne";
	pugi::xml_document sceneDoc;
	pugi::xml_parse_result result = sceneDoc.load_file(path.c_str());
	if(!result)
	{
		std::cerr <<"Failed to open scene graph " << path << std::endl;
		std::cerr << "Reason: " << result.description() << std::endl;
		return;
	}

	//get root node and iterate over children
	pugi::xml_node rootNode, meshNode;
	if(!(rootNode = sceneDoc.child("scene"))
		|| !(meshNode = rootNode.child("mesh")))
	{
		std::cerr << "Invalid mesh scene data..." << std::endl;
		std::cerr << "Scene graph not created." << std::endl;
		return;
	}

	while(meshNode)
	{
		std::string meshName = meshNode.attribute("name").as_string();
		assert(!meshName.empty());
		assert(meshNode.child("position"));
		assert(meshNode.child("rotation"));
		assert(meshNode.child("scale"));

		pugi::xml_node posNode, rotNode, scaleNode;
		posNode = meshNode.child("position");
		rotNode = meshNode.child("rotation");
		scaleNode = meshNode.child("scale");

		sf::Vector3f position, rotation, scale;
		position.x = posNode.attribute("x").as_float();
		position.y = posNode.attribute("y").as_float();
		position.z = posNode.attribute("z").as_float();

		rotation.x = rotNode.attribute("x").as_float();
		rotation.y = rotNode.attribute("y").as_float();
		rotation.z = rotNode.attribute("z").as_float();

		scale.x = scaleNode.attribute("x").as_float();
		scale.y = scaleNode.attribute("y").as_float();
		scale.z = scaleNode.attribute("z").as_float();

		//load mesh
		StaticMeshNode* staticMesh = AddStaticMesh(m_meshResource.Get(meshPath + meshName, ml::FileType::Obj));
		staticMesh->SetPosition(position);
		staticMesh->SetRotation(rotation);
		staticMesh->SetScale(scale);

		//load material data if found
		pugi::xml_node matNode;
		if(matNode = meshNode.child("material"))
		{
			std::string matName = materialPath + mapName + "/" + matNode.text().as_string();
			pugi::xml_document matDoc;
			result = matDoc.load_file(matName.c_str());
			if(!result)
			{
				std::cerr << "Failed to load material file " << matName << std::endl;
				std::cerr << result.description() << std::endl;
			}
			else
			{
				assert(matDoc.child("matdef"));
				pugi::xml_node defNode = matDoc.child("matdef");
				assert(defNode.child("shader"));
				assert(defNode.child("map"));

				//get texture info
				pugi::xml_node texNode = defNode.child("map");
				if(texNode.attribute("diffuse")) 
					staticMesh->SetTexture(m_textureResource.Get(texturePath + mapName + "/" + texNode.attribute("diffuse").as_string()));

				//parse shader types
				std::string shaderType = defNode.child("shader").attribute("type").as_string();
				if(shaderType == "phong")
				{
					staticMesh->CreateShader(ShaderType::PhongShade);
				}
				else if(shaderType == "normal")
				{
					assert(texNode.attribute("normal"));
					assert(texNode.attribute("mask"));
					
					ShaderPtr normalShader = staticMesh->CreateShader(ShaderType::NormalMap);
					normalShader->setParameter("colorMap", m_textureResource.Get(texturePath + mapName + "/" + texNode.attribute("diffuse").as_string()));
					normalShader->setParameter("normalMap", m_textureResource.Get(texturePath + texNode.attribute("normal").as_string()));
					normalShader->setParameter("reflectMap", m_textureResource.Get(reflectMapPath));
					normalShader->setParameter("maskMap", m_textureResource.Get(texturePath + texNode.attribute("mask").as_string()));
				}

				//TODO check material properties for self-illum flag and only create shader if necessary
				ShaderPtr sillum = staticMesh->CreateShader(ShaderType::SelfIllum);
				sillum->setParameter("colour", m_textureResource.Get(texturePath + mapName + "/" + texNode.attribute("diffuse").as_string()));
				sillum->setParameter("mask", m_textureResource.Get(texturePath + texNode.attribute("mask").as_string()));

				if(pugi::xml_node matDataNode = defNode.child("material"))
				{
					//create opengl material
					assert(matDataNode.child("ambient"));
					assert(matDataNode.child("diffuse"));
					assert(matDataNode.child("specular"));
					assert(matDataNode.child("emissive"));
					assert(matDataNode.child("shininess"));

					sf::Color ambient(matDataNode.child("ambient").attribute("r").as_uint(),
									matDataNode.child("ambient").attribute("g").as_uint(),
									matDataNode.child("ambient").attribute("b").as_uint());

					sf::Color diffuse(matDataNode.child("diffuse").attribute("r").as_uint(),
									matDataNode.child("diffuse").attribute("g").as_uint(),
									matDataNode.child("diffuse").attribute("b").as_uint());

					sf::Color specular(matDataNode.child("specular").attribute("r").as_uint(),
									matDataNode.child("specular").attribute("g").as_uint(),
									matDataNode.child("specular").attribute("b").as_uint());

					sf::Color emissive(matDataNode.child("emissive").attribute("r").as_uint(),
									matDataNode.child("emissive").attribute("g").as_uint(),
									matDataNode.child("emissive").attribute("b").as_uint());

					float shininess = matDataNode.child("shininess").attribute("value").as_float();

					Material meshMaterial(ambient, diffuse, specular, emissive);
					meshMaterial.SetShininess(shininess);
					staticMesh->SetMaterial(meshMaterial);
				}
			}

		}
		else
		{
			std::cerr << "Material not found for " << meshName << std::endl;
		}
		meshNode = meshNode.next_sibling("mesh"); //move to next mesh
	}
	//parse any light positions and add lights to the scene
	if(pugi::xml_node lightNode = rootNode.child("light"))
	{
		while(lightNode)
		{
			assert(lightNode.attribute("x"));
			assert(lightNode.attribute("y"));
			assert(lightNode.attribute("z"));
			
			sf::Vector3f lightPos(lightNode.attribute("x").as_float(),
								lightNode.attribute("y").as_float(),
								lightNode.attribute("z").as_float());

			LightPtr l = AddLight();
			l->SetPosition(lightPos);

			lightNode = lightNode.next_sibling("light");
		}
	}
	else
	{
		std::cerr << "Warning: no lighting data found in scene" << std::endl;
	}
}

void MeshScene::DrawSelfIllum(sf::RenderTarget& rt)
{
	sf::View v = rt.getView();
	rt.setView(m_illumTexture.getView());
	rt.draw(m_illumSprite);
	rt.setView(v);
}

//private


void MeshScene::draw(sf::RenderTarget& rt, sf::RenderStates states) const
{
	sf::View v = rt.getView();
	rt.setView(m_renderTexture.getView());
	rt.draw(m_renderSprite, states);
	rt.setView(v);
}