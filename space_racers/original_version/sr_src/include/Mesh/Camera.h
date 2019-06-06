///base class for 3D camera used by Mesh::Scene///
#ifndef CAMERA_H_
#define CAMERA_H_

#include <SFML/System/Vector3.hpp>

namespace ml
{
	class Camera
	{
	public:
		Camera();
		virtual ~Camera();

		//sets the camera's postion
		void SetPosition(const sf::Vector3f& position);
		void SetPosition(float x, float y, float z);
		//gets the camera's current position
		const sf::Vector3f& GetPosition() const;
		//moves the camera, equivalent to adding amount to current position
		void Move(const sf::Vector3f& amount);
		void Move(float x, float y, float z);
		//sets the rotation of the camera
		void SetRotation(const sf::Vector3f& rotation);
		void SetRotation(float x, float y, float z);
		//gets the camera's current rotation
		const sf::Vector3f& GetRotation() const;
		//rotate the camera by adding amount to the current rotation
		void Rotate(const sf::Vector3f& amount);
		void Rotate(float x, float y, float z);
		//TODO LookAt(sf::Vector3f world point)
		//sets the camera FOV in degrees
		void SetFOV(float angle);
		//returns the camera's current FOV
		float GetFOV() const;
		//sets the camera's aspect ratio, eg 1.77f for 16:9
		void SetAspectRatio(float ratio);
		//gets the camera's current aspect ratio
		float GetAspectRatio() const;
		//sets the near plane distance of viewing frustum
		void SetNearValue(float distance);
		//gets the distance of the near plane of the viewing frustum
		float GetNearValue() const;
		//sets the far plane distance of the viewing frustum
		void SetFarValue(float distance);
		//gets the distance to the far plane of the viewing frustum
		float GetFarValue() const;
		//updates any camera properties, for example via real time input
		virtual void Update(float dt);
		//sets the view matrix for rendering the scene via the camera
		virtual void Draw();

	private:
		float m_FOV;
		float m_nearValue, m_farValue;
		float m_aspectRatio;

		sf::Vector3f m_position, m_rotation;

		void m_ClampRotation();
	};
}

#endif //CAMERA_H_