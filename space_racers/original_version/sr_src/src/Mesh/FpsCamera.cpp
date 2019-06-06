///source for fps camera class
#include <Mesh/FpsCamera.h>
#include <Mesh/MeshHelpers.h>

using namespace ml;
  
//ctor
FpsCamera::FpsCamera(sf::RenderWindow& renderWindow)
    :   Camera		(),
	m_renderWindow	(renderWindow),
	m_mouseSpeed	(0.3f),
	m_inputEnabled	(true),
	//--default key bindings--//
	m_keyForward	(sf::Keyboard::W),
	m_keyBackward	(sf::Keyboard::S),
	m_strafeLeft	(sf::Keyboard::A),
	m_strafeRight	(sf::Keyboard::D),
	m_pitchUp		(sf::Keyboard::Up),
	m_pitchDown		(sf::Keyboard::Down),
	m_yawLeft		(sf::Keyboard::Left),
	m_yawRight		(sf::Keyboard::Right)
{
	m_screenCentre = static_cast<sf::Vector2f>(m_renderWindow.getSize() / 2u);
}


//public
void FpsCamera::StrafeLeft()
{
    float yRad = GetRotation().y / 180.f * Helpers::pi;

    sf::Vector3f pos = GetPosition();
    pos.x += std::cos(yRad);
    pos.z += std::sin(yRad);
    SetPosition(pos);
}

void FpsCamera::StrafeRight()
{
    float yRad = GetRotation().y / 180.f * Helpers::pi;
    sf::Vector3f pos = GetPosition();
    pos.x -= std::cos(yRad);
    pos.z -= std::sin(yRad);
    SetPosition(pos);
}

void FpsCamera::MoveForward()
{
    float yRad = GetRotation().y / 180.f * Helpers::pi;
    float xRad = GetRotation().x / 180.f * Helpers::pi;
    sf::Vector3f pos = GetPosition();
    pos.x -= std::sin(yRad);
    pos.z += std::cos(yRad);
    pos.y += std::sin(xRad);
    SetPosition(pos);
}

void FpsCamera::MoveBackward()
{
    float yRad = GetRotation().y / 180.f * Helpers::pi;
    float xRad = GetRotation().x / 180.f * Helpers::pi;
    sf::Vector3f pos = GetPosition();
    pos.x += std::sin(yRad);
    pos.z -= std::cos(yRad);
    pos.y -= std::sin(xRad);
    SetPosition(pos);
}



void FpsCamera::Update(float dt)
{
	const float speed = 100.f;
		
	if(sf::Keyboard::isKeyPressed(m_pitchUp))
		Pitch(-speed * dt);

	if(sf::Keyboard::isKeyPressed(m_pitchDown))
		Pitch(speed * dt);

	if(sf::Keyboard::isKeyPressed(m_yawLeft))
        Yaw(-speed * dt);

	if(sf::Keyboard::isKeyPressed(m_yawRight))
        Yaw(speed * dt);

	if(sf::Keyboard::isKeyPressed(m_keyForward))
        MoveForward();

	if(sf::Keyboard::isKeyPressed(m_keyBackward))
        MoveBackward();

	if(sf::Keyboard::isKeyPressed(m_strafeRight))
        StrafeRight();

    if(sf::Keyboard::isKeyPressed(m_strafeLeft))
        StrafeLeft();

	//update mouse look
	if(m_inputEnabled)
	{
		sf::Vector2f mousePos = static_cast<sf::Vector2f>(sf::Mouse::getPosition(m_renderWindow)) - m_screenCentre;		
		mousePos *= m_mouseSpeed;
		Rotate(sf::Vector3f(mousePos.y, mousePos.x, 0.f));
		sf::Mouse::setPosition(sf::Vector2i(m_screenCentre), m_renderWindow);
	}

}

void FpsCamera::Pitch(float amount)
{
	Rotate(amount, 0.f, 0.f);
}

void FpsCamera::Yaw(float amount)
{
	Rotate(0.f, amount, 0.f);
}

void FpsCamera::Roll(float amount)
{
	Rotate(0.f, 0.f, amount);
}

void FpsCamera::SetKey(Input input, sf::Keyboard::Key key)
{
	switch(input)
	{
	case FORWARD:
		m_keyForward = key;
		break;
	case BACKWARD:
		m_keyBackward = key;
		break;
	case STRAFE_LEFT:
		m_strafeLeft = key;
		break;
	case STRAFE_RIGHT:
		m_strafeRight = key;
		break;
	case PITCH_UP:
		m_pitchUp = key;
		break;
	case PITCH_DOWN:
		m_pitchDown = key;
		break;
	case YAW_LEFT:
		m_yawLeft = key;
		break;
	case YAW_RIGHT:
		m_yawRight = key;
		break;
	}
}

void FpsCamera::SetInputEnabled(bool b)
{
	m_inputEnabled = b;
	m_renderWindow.setMouseCursorVisible(!b);
}
