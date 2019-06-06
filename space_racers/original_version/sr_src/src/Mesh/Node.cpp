///source for 3d scene node base class///
#include <Mesh/Node.h>
#include <Mesh/Light.h>
#include <assert.h>

using namespace ml;

namespace
{
	const int MAX_LIGHTS = 8;
}

//ctor
Node::Node()
	: m_parent	(nullptr),
    m_scale		(sf::Vector3f(1.f, 1.f, 1.f))/*,
	m_sceneNode	(nullptr)*/
{

}

Node::~Node(){}

//public
void Node::SetPosition(const sf::Vector3f& position)
{
	m_position = position;
}

void Node::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

const sf::Vector3f& Node::GetPosition() const
{
	return m_position;
}

void Node::Move(const sf::Vector3f& amount)
{
	m_position += amount;
}

void Node::Move(float x, float y, float z)
{
	m_position.x += x;
	m_position.y += y;
	m_position.z += z;
}

void Node::SetRotation(const sf::Vector3f& rotation)
{
	m_rotation  = rotation;
}

void Node::SetRotation(float x, float y, float z)
{
	m_rotation.x = x;
	m_rotation.y = y;
	m_rotation.z = z;
	m_ClampRotation();
}

const sf::Vector3f& Node::GetRotation() const
{
	return m_rotation;
}

void Node::Rotate(const sf::Vector3f& amount)
{
	m_rotation += amount;
	m_ClampRotation();
}

void Node::Rotate(float x, float y, float z)
{
	m_rotation.x += x;
	m_rotation.y += y;
	m_rotation.z += z;
	m_ClampRotation();
}

void Node::SetScale(const sf::Vector3f& scale)
{
	m_scale = scale;
}

const sf::Vector3f& Node::GetScale() const
{
	return m_scale;
}

void Node::Scale(const sf::Vector3f& amount)
{
	m_scale += amount;
}

void Node::SetSceneScale(float scale)
{
	//if(m_sceneNode)
	//{
	//	m_position *= scale;
	//	m_scale *= scale;
	//}
}


//void Node::SetSceneNode(Game::SceneNode& node)
//{
//	m_sceneNode = &node;
//}

void Node::AddChild(NodePtr node)
{
	node->m_parent = this;
	m_children.push_back(std::move(node));
}

NodePtr Node::RemoveChild(const Node& child)
{
	auto result = std::find_if(m_children.begin(),
								m_children.end(),
								[&] (NodePtr& p)->bool {return p.get() == &child;});
	
	//make sure we actually found something
	assert(result != m_children.end());

	NodePtr retVal = std::move(*result); //save pointer to any child we may have found so we can return it
	retVal->m_parent = nullptr;
	m_children.erase(result);
	return retVal;
}

void Node::SetMaterial(const Material& material)
{
	m_material = material;
}

void Node::ApplyLighting(Lights& lights)
{
    // Get nearest lights
    for(auto&& l : lights)
		l->CalcBrightness(m_position);

    std::sort(lights.begin(), lights.end(),
		[](LightPtr& l1, LightPtr& l2)->bool { return l1->GetBrightness() > l2->GetBrightness(); }
	);

	//clamp max number of lights
    int lightCount = lights.size();
    sf::Uint32 maxLights = std::min(MAX_LIGHTS, lightCount);
	//create gl buffers for each light
    for (sf::Uint8 i = 0; i < maxLights; ++i)
    {
        Light& l = *lights[i];

        glEnable(GL_LIGHT0 + i);

        const sf::Color& ambientColour = l.GetAmbientColour();
        GLfloat ambient[] = { static_cast<GLfloat>(ambientColour.r) / 255.f,
                                static_cast<GLfloat>(ambientColour.g) / 255.f,
                                static_cast<GLfloat>(ambientColour.b) / 255.f,
                                static_cast<GLfloat>(ambientColour.a) / 255.f };

        const sf::Color& diffuseColour = l.GetDiffuseColour();
        GLfloat diffuse[] = { static_cast<GLfloat>(diffuseColour.r) / 255.f,
                                static_cast<GLfloat>(diffuseColour.g) / 255.f,
                                static_cast<GLfloat>(diffuseColour.b) / 255.f,
                                static_cast<GLfloat>(diffuseColour.a) / 255.f };

        const sf::Color& specularColour = l.GetSpecularColour();
        GLfloat specular[] = { static_cast<GLfloat>(specularColour.r) / 255.f,
                                static_cast<GLfloat>(specularColour.g) / 255.f,
                                static_cast<GLfloat>(specularColour.b) / 255.f,
                                static_cast<GLfloat>(specularColour.a) / 255.f };

        const sf::Vector3f& pos = l.GetPosition();
        GLfloat position[] = { pos.x, pos.y, pos.z, 0.f };

        glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, diffuse);
        glLightfv(GL_LIGHT0 + i, GL_SPECULAR, specular);
        glLightfv(GL_LIGHT0 + i, GL_POSITION, position);

		//draws a line to the light
        /*glBegin(GL_LINES);
            glColor3ub(255, 255, 255);
            glVertex3f(pos.x, pos.y, pos.z);
            glVertex3f(0, 0, 0);
        glEnd();*/
    }
}

void Node::Update(float dt)
{
	m_UpdateSelf(dt);
	for(auto&& c : m_children)
		c->Update(dt);
}

void Node::Draw(RenderPass pass)
{
	m_DrawSelf(pass);
	for(auto&& c : m_children)
		c->Draw(pass);
}

//protected
void Node::m_ApplyTransform()
{
    glTranslatef(m_position.x, m_position.y, m_position.z);
    glRotatef(m_rotation.y, 0, 1, 0);
    glRotatef(m_rotation.z, 0, 0, 1);
    glRotatef(m_rotation.x, 1, 0, 0);
    glScalef(m_scale.x, m_scale.y, m_scale.z);
}

void Node::m_ApplyMaterial()
{
    //set gl material buffers
	sf::Color c = m_material.GetAmbientColour();
	GLfloat ambient[] = { static_cast<GLfloat>(c.r) / 255.f,
                            static_cast<GLfloat>(c.g) / 255.f,
                            static_cast<GLfloat>(c.b) / 255.f,
                            static_cast<GLfloat>(c.a) / 255.f };

	c = m_material.GetDiffuseColour();
    GLfloat diffuse[] = { static_cast<GLfloat>(c.r) / 255.f,
                            static_cast<GLfloat>(c.g) / 255.f,
                            static_cast<GLfloat>(c.b) / 255.f,
                            static_cast<GLfloat>(c.a) / 255.f };

	c = m_material.GetSpecularColour();
    GLfloat specular[] = { static_cast<GLfloat>(c.r) / 255.f,
                            static_cast<GLfloat>(c.g) / 255.f,
                            static_cast<GLfloat>(c.b) / 255.f,
                            static_cast<GLfloat>(c.a) / 255.f };

	c = m_material.GetEmissiveColour();
    GLfloat emissive[] = { static_cast<GLfloat>(c.r) / 255.f,
                            static_cast<GLfloat>(c.g) / 255.f,
                            static_cast<GLfloat>(c.b) / 255.f,
                            static_cast<GLfloat>(c.a) / 255.f};

	//set shininess
	GLfloat shininess = m_material.GetShininess();

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emissive);
	glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    if(m_material.Smooth())
        glShadeModel(GL_SMOOTH);
    else
        glShadeModel(GL_FLAT);
}


void Node::m_UpdateSelf(float dt)
{
	//implement this in derived classes
}

void Node::m_DrawSelf(RenderPass pass)
{
	//implement this in derived classes
}

//Game::SceneNode* Node::m_GetSceneNode() const
//{
//	return m_sceneNode;
//}

//private
void Node::m_ClampRotation()
{
	if(m_rotation.x > 360.f) m_rotation.x -= 360.f;
	else if (m_rotation.x < -360.f) m_rotation.x += 360.f;

	if(m_rotation.y > 360.f) m_rotation.y -= 360.f;
	else if (m_rotation.y < -360.f) m_rotation.y += 360.f;

	if(m_rotation.z > 360.f) m_rotation.z -= 360.f;
	else if (m_rotation.z < -360.f) m_rotation.z += 360.f;
}