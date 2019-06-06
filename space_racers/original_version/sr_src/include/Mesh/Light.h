///creates a light in 3D space///
#ifndef LIGHT_H_
#define LIGHT_H_

#include <SFML/System/String.hpp>
#include <SFML/System/Vector3.hpp>
#include <SFML/Graphics/Color.hpp>

#include <vector>
#include <memory>

namespace ml
{
    class Light
    {
        public:
			//class representing a light source
            Light();
			//set the light position in 3d space
            void SetPosition(const sf::Vector3f& position);
			void SetPosition(float x, float y, float z);
			//get the light's current position in 3d space
            const sf::Vector3f& GetPosition() const;
			//move the light. Equivalent of adding move parameter to current position
            void Move(const sf::Vector3f& amount);
			void Move(float x, float y, float z);
			//sets the light's ambient colour - ie the shadow colour
            void SetAmbientColour(const sf::Color& colour);
			//returns the light's current ambient colour value
            const sf::Color& GetAmbientColour() const;
			//sets the light's diffuse colour - ie the colour which it casts on illuminated objects
            void SetDiffuseColour(const sf::Color& colour);
			//returns the light's current diffuse colour
            const sf::Color& GetDiffuseColour() const;
			//sets the light's specular colour - ie the colour which affects highlights of glossy materials
            void SetSpecularColour(const sf::Color& colour);
			//returns the light's current specular colour
            const sf::Color& GetSpecularColour() const;
			//calculates the brightness of the light from the distance of a given position
            void CalcBrightness(const sf::Vector3f& position);
			//returns the light's brightness
            float GetBrightness() const;

        private:
            sf::Vector3f m_position;
            float m_brightness;
            sf::Color m_ambientColour, m_diffuseColour, m_specularColour;
    };

	typedef std::shared_ptr<Light> LightPtr;
	typedef std::vector<LightPtr> Lights;
}

#endif //LIGHT_H_