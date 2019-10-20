/*********************************************************************

Copyright 2019 Matt Marchant

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*********************************************************************/

#pragma once

#include <xyginext/ecs/System.hpp>
#include <xyginext/core/ConsoleClient.hpp>

#include <SFML/Graphics/Glsl.hpp>
#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

#include <SFML/System/Clock.hpp>

#include <vector>
#include <array>
#include <map>

namespace sf
{
    class Shader;
}

namespace xy
{
    class ShaderResource;
    class TextureResource;
}

class DayNightSystem final : public xy::System, public sf::Drawable
{
public:
    DayNightSystem(xy::MessageBus&, xy::ShaderResource&, xy::TextureResource&);

    void handleMessage(const xy::Message&) override;

    void process(float) override;

    const sf::Texture& getBufferTexture() const { return m_bufferTexture.getTexture(); }

    void setStormLevel(std::int32_t);

    void prepShaders();

private:

    float m_fadeInVolume; //used to fade in the looped sounds once the curtain is raised
    float m_targetVolume;

    std::vector<sf::Shader*> m_shaders;
    sf::Shader* m_groundShader;
    sf::Glsl::Vec3 m_sunDirection;

    std::vector<float> m_waveTable;
    std::size_t m_currentIndex;
    std::size_t m_currentIndexTwoFold;

    sf::Transform m_cameraTransform;
    sf::Sprite m_skySprite;
    mutable sf::Shader* m_skyShader;

    sf::Sprite m_sunSprite;
    sf::Sprite m_moonSprite;
    sf::Shader* m_moonShader;
    float m_dayNumber;

    sf::Shader* m_shadowShader;

    sf::Sprite m_cloudSprite;

    sf::Sprite m_starSprite;
    sf::Clock m_dayTimer;

    std::vector<sf::Shader*> m_lampShaders;
    std::map<std::uint32_t, std::int32_t> m_lampUniforms;
    sf::Shader* m_sunsetShader;
    std::pair<std::uint32_t, std::int32_t> m_sunsetUniform;

    sf::Sprite m_bufferSprite;
    mutable sf::RenderTexture m_bufferTexture;

    bool m_isDayTime;

    float m_flareAmount;
    float m_targetFlareAmount;
    std::vector<float> m_flareFlickerTable;
    std::size_t m_flickerIndex;

    float m_stormAmount;
    float m_targetStormAmount;

    bool m_doLightning;
    struct Lightning final
    {
        static constexpr std::array<float, 26> times =
        {
            1.1f, //off
            0.05f, //on
            0.1f, //off
            0.05f, //on
            28.f, //off

            0.05f, //on
            17.15f, //off
            0.05f, //on
            0.1f, //off
            0.06f, //on
            26.25f, //off
            0.05f, //on
            32.f, //off

            0.05f, //on
            22.1f, //off
            0.05f, //on
            27.f, //off
            0.05f, //on
            0.15f, //off
            0.05f, //on
            0.1f, //off
            0.06f, //on
            27.25f, //off
            0.05f, //on
            34.f, //off
            0.01f //on
        };
        bool visible = false;
        sf::Clock timer;
        std::size_t flashIndex = 0;
        void reset()
        {
            visible = false;
            flashIndex = 0;
            timer.restart();
        }
    }m_lightning;
    bool updateLightning();

    void setTimeOfDay(float);

    void draw(sf::RenderTarget&, sf::RenderStates) const override;
};
