///a camera with realtime input which creates an first person style view///
// controllable with configurable mouse / keyboard
#ifndef FPS_CAMERA_H_
#define FPS_CAMERA_H_

#include <Mesh/Camera.h>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window.hpp>

namespace ml
{
	class FpsCamera final : public Camera
	{
	public:
		enum Input
		{
			FORWARD,
			BACKWARD,
			STRAFE_LEFT,
			STRAFE_RIGHT,
			PITCH_UP,
			PITCH_DOWN,
			YAW_LEFT,
			YAW_RIGHT
		};

		//represents a first person camera by adding keyboard input to camera class
        FpsCamera(sf::RenderWindow& renderWindow);

        void    StrafeLeft();
        void    StrafeRight();

		//moves into the scene
        void    MoveForward();
		//moves out of the scene
        void    MoveBackward();

		void Pitch(float amount = 1.f);
		void Yaw(float amount = 1.f);
		void Roll(float amount = 1.f);

		//updates controller input
		void Update(float dt);
		//sets mouse speed multiplier
		void SetMouseSpeed(float);

		//allows assigning keys for control
		void SetKey(FpsCamera::Input input, sf::Keyboard::Key key);

		//enable / disable mouse input so we can override using a ui or something
		void SetInputEnabled(bool b);
		bool GetInputEnabled() const{return m_inputEnabled;};

    private:
		sf::RenderWindow& m_renderWindow;
		sf::Vector2f m_screenCentre;
		float m_mouseSpeed;

		sf::Keyboard::Key m_keyForward, m_keyBackward;
		sf::Keyboard::Key m_strafeLeft, m_strafeRight;
		sf::Keyboard::Key m_pitchUp, m_pitchDown, m_yawLeft, m_yawRight;

		bool m_inputEnabled;
	};
}

#endif