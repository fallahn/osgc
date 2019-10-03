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

#include "DayNightSystem.hpp"
#include "ResourceIDs.hpp"
#include "MessageIDs.hpp"
#include "CommandIDs.hpp"
#include "GlobalConsts.hpp"
#include "SeagullSystem.hpp"
#include "FoliageSystem.hpp"
#include "FlappySailSystem.hpp"
#include "MixerChannels.hpp"

#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/resources/Resource.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/AudioEmitter.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/systems/CommandSystem.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/util/Wavetable.hpp>
#include <xyginext/util/Random.hpp>
#include <xyginext/util/Math.hpp>

#include <xyginext/audio/Mixer.hpp>

#include <SFML/Graphics/Shader.hpp>
#include <SFML/Graphics/RenderStates.hpp>
#include <SFML/Graphics/RenderTarget.hpp>

/*
Day night cycle, based on a precomputed sine wave runs at double time
so the normalised value of the sine wave equates to:
0 - 0.25 night time
0.25 - 0.5 day time
0.5 - 0.75 night time
0.75 - 1 day time
*/

namespace
{
    //these are normalised HSV colours, white, orange, blue
    //0, 0, 100
    //30, 56, 100
    //239, 29, 16
    //except for the angle which remains 0 - 360
    std::array<sf::Glsl::Vec4, 2u> skyColours = 
    {
        sf::Glsl::Vec4(0.f, 0.f, 1.f, 1.f),
        {-121.f, 29.f / 100.f, 16.f / 100.f, 1.f}
    };

    sf::Glsl::Vec4 lerp(sf::Glsl::Vec4 a, sf::Glsl::Vec4 b, float t)
    {
        return { a.x * (1.f - t) + b.x * t,
                a.y * (1.f - t) + b.y * t,
                a.z * (1.f - t) + b.z * t,
                1.f };
    }

    sf::Glsl::Vec4 HSVtoRGB(sf::Glsl::Vec4 input)
    {
        if (input.y <= 0)
        {
            //no saturation, ie black and white
            return { input.z, input.z, input.z, 1.f };
        }

        float hh = input.x;
        if (hh >= 360.f)
        {
            hh -= 360.f;
        }
        else if (hh < 0)
        {
            hh += 360.f;
        }
        hh /= 60.f;

        int i = static_cast<int>(hh);
        float ff = hh - i;

        float p = input.z * (1.f - input.y);
        float q = input.z * (1.f - (input.y * ff));
        float t = input.z * (1.f - (input.y * (1.f - ff)));

        switch (i)
        {
        case 0:
            return { input.z, t, p , 1.f };
        case 1:
            return { q, input.z, p, 1.f };
        case 2:
            return { p, input.z, t, 1.f };
        case 3:
            return { p, q, input.z, 1.f };
        case 4:
            return { t, p , input.z, 1.f };
        case 5:
        default:
            return { input.z, p, q, 1.f };
        }
        return {};
    }

    const float StormTransitionTime = 2.f;
    const sf::Glsl::Vec4 StormColour(0.4f, 0.4f, 0.4f, 1.f); //subtractive - so bigger == darker
    const sf::Glsl::Vec4 WhiteColour(1.f, 1.f, 1.f, 1.f);

    sf::Glsl::Vec4 operator * (const sf::Glsl::Vec4& v, float f)
    {
        return { v.x*f, v.y*f, v.z*f, v.w }; //look this is a kludge OK? I know alpha isn't updated.
    }

    sf::Glsl::Vec4& operator *= (sf::Glsl::Vec4& l, const sf::Glsl::Vec4& r)
    {
        l.x *= r.x;
        l.y *= r.y;
        l.z *= r.z;
        l.w *= r.w;
        return l;
    }

    sf::Glsl::Vec4 operator - (const sf::Glsl::Vec4& l, const sf::Glsl::Vec4& r)
    {
        return { l.x - r.x, l.y - r.y, l.z - r.z, l.w };
    }

    sf::Glsl::Vec4& operator += (sf::Glsl::Vec4& l, const sf::Glsl::Vec4& r)
    {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        l.w += r.w;
        return l;
    }

    const float FlareTransitionTime = 3.f;
    const sf::Glsl::Vec4 FlareColour(255.f / 255.f, 143.f / 255.f, 246.f / 255.f, 1.f);
    const float AudioFadeTime = 12.f;
}

DayNightSystem::DayNightSystem(xy::MessageBus& mb, xy::ShaderResource& sr, xy::TextureResource& tr)
    : xy::System            (mb, typeid(DayNightSystem)),
    m_fadeInVolume          (0.f),
    m_targetVolume          (1.f),
    m_groundShader          (nullptr),
    m_sunDirection          (-2.f, -1.f, 2.4f),
    m_currentIndex          (0),
    m_currentIndexTwoFold   (0),
    m_skyShader             (nullptr),
    m_moonShader            (nullptr),
    m_dayNumber             (static_cast<float>(xy::Util::Random::value(0, 7))),
    m_shadowShader          (nullptr),
    m_sunsetShader          (nullptr),
    m_isDayTime             (true),
    m_flareAmount           (0.f),
    m_targetFlareAmount     (0.f),
    m_flickerIndex          (0),
    m_stormAmount           (0.f),
    m_targetStormAmount     (0.f),
    m_doLightning           (false)
{
    xy::AudioMixer::setPrefadeVolume(m_fadeInVolume, MixerChannel::FX);
    xy::AudioMixer::setPrefadeVolume(m_fadeInVolume, MixerChannel::Music);

    m_shaders.push_back(&sr.get(ShaderID::LandShader));
    m_shaders.push_back(&sr.get(ShaderID::PlaneShader));
    m_shaders.push_back(&sr.get(ShaderID::SkyShader));
    m_shaders.push_back(&sr.get(ShaderID::SpriteShader));
    m_shaders.push_back(&sr.get(ShaderID::SpriteShaderCulled));
    m_shaders.push_back(&sr.get(ShaderID::MoonShader));

    m_lampShaders.push_back(&sr.get(ShaderID::LandShader));
    m_lampShaders.push_back(&sr.get(ShaderID::PlaneShader));
    m_lampShaders.push_back(&sr.get(ShaderID::SpriteShader));
    m_lampShaders.push_back(&sr.get(ShaderID::SpriteShaderCulled));

    m_groundShader = &sr.get(ShaderID::LandShader);
    m_skyShader = &sr.get(ShaderID::SkyShader);
    m_moonShader = &sr.get(ShaderID::MoonShader);
    m_shadowShader = &sr.get(ShaderID::ShadowShader);

    //used to interpolate colours
    m_waveTable = xy::Util::Wavetable::sine(0.1f);
    setTimeOfDay(Global::DayCycleOffset);

    tr.get("assets/images/background.png").setRepeated(true);
    m_skySprite.setTexture(tr.get("assets/images/background.png"));
    float scale = xy::DefaultSceneSize.x / m_skySprite.getTextureRect().width;
    m_skySprite.setScale(scale, scale);

    tr.get("assets/images/clouds.png").setRepeated(true);
    m_cloudSprite.setTexture(tr.get("assets/images/clouds.png"));
    m_cloudSprite.setScale(m_skySprite.getScale());

    m_starSprite.setTexture(tr.get("assets/images/stars.png"));
    m_starSprite.setScale(4.f, 4.f);

    m_sunSprite.setTexture(tr.get("assets/images/sun_moon.png"));
    auto rect = m_sunSprite.getTextureRect();
    rect.width /= 2;
    rect.left = rect.width;
    m_sunSprite.setTextureRect(rect);
    m_sunSprite.setOrigin({ rect.width / 2.f, rect.height / 2.f });
    m_sunSprite.setPosition(xy::DefaultSceneSize.x / 4.f, 0.f);

    m_moonSprite.setTexture(tr.get("assets/images/moon.png"));
    rect.left = 0;
    m_moonSprite.setTextureRect(rect);
    m_moonSprite.setPosition(xy::DefaultSceneSize.x * 0.75f, 0.f);
    m_moonSprite.setOrigin({ rect.width / 2.f, rect.height / 2.f });
    m_moonShader->setUniform("u_texture", *m_moonSprite.getTexture());

    m_bufferTexture.create(1920, 1080);
    m_bufferSprite.setTexture(m_bufferTexture.getTexture());
    m_bufferSprite.setOrigin(xy::DefaultSceneSize / 2.f);

    m_sunsetShader = &sr.get(ShaderID::SunsetShader);

    m_flareFlickerTable = xy::Util::Wavetable::sine(6.f);
    for (auto& f : m_flareFlickerTable)
    {
        f += 1.f;
        f /= 2.f;
        f *= 0.2f;
        f += 0.8f;
    }

    requireComponent<xy::Camera>();
}

//public
void DayNightSystem::handleMessage(const xy::Message& msg)
{
    switch (msg.id)
    {
    default: break;
    case MessageID::MapMessage:
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::DayNightUpdate)
        {
            setTimeOfDay(data.value);
        }
        else if (data.type == MapEvent::FlareLaunched)
        {
            m_targetFlareAmount = 1.f;
        }
    }
        break;
    }
}

void DayNightSystem::process(float dt)
{
    //interpolates movement between index updates
    std::size_t nextIndex = (m_currentIndexTwoFold + 2) % m_waveTable.size();
    float interpOffset = (m_waveTable[nextIndex] - m_waveTable[m_currentIndexTwoFold]) * m_dayTimer.getElapsedTime().asSeconds();
    
    //sets the amount of light lamps should cast
    float lightAmount = ((m_waveTable[m_currentIndexTwoFold] + interpOffset) + 1.f) / 2.f;
    //xy::Console::printStat("light amount", std::to_string(lightAmount));
    for (auto shader : m_lampShaders)
    {
        shader->setUniform("u_lightAmount", lightAmount);
    }
    m_sunsetShader->setUniform("u_lightness", lightAmount * 0.5f);
    m_shadowShader->setUniform("u_amount", lightAmount);

    //update the lights of ships at sea
    xy::Command shipCommand;
    shipCommand.targetFlags = CommandID::ShipLight;
    shipCommand.action = [lightAmount](xy::Entity e, float)
    {
        sf::Color c(255, 255, 255, static_cast<sf::Uint8>(255.f * lightAmount));
        e.getComponent<xy::Sprite>().setColour(c);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(shipCommand);

    //check if we're day or night based on light level
    if (m_isDayTime && lightAmount > 0.5f)
    {
        m_isDayTime = false;
        
        xy::Command cmd;
        cmd.targetFlags = CommandID::Bird;
        cmd.action = [](xy::Entity ent, float)
        {
            ent.getComponent<Seagull>().enabled = false;
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
    else if (!m_isDayTime && lightAmount < 0.5f)
    {
        m_isDayTime = true;
        
        xy::Command cmd;
        cmd.targetFlags = CommandID::Bird;
        cmd.action = [](xy::Entity ent, float)
        {
            ent.getComponent<Seagull>().enabled = true;
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);
    }

    if (m_fadeInVolume < m_targetVolume)
    {
        m_fadeInVolume = std::min(m_targetVolume, m_fadeInVolume + (dt / AudioFadeTime));

        xy::AudioMixer::setPrefadeVolume(m_fadeInVolume, MixerChannel::FX);
        xy::AudioMixer::setPrefadeVolume(m_fadeInVolume, MixerChannel::Music);
    }

    //update day/night sounds
    if (!m_isDayTime)
    {
        float volume = xy::Util::Math::clamp((lightAmount - 0.5f) * 8.f, 0.f, 1.f);

        xy::Command cmd;
        cmd.targetFlags = CommandID::DaySound;
        cmd.action = [volume](xy::Entity ent, float)
        {
            ent.getComponent<xy::AudioEmitter>().setVolume(ent.getComponent<float>() * (1.f - volume));
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = CommandID::NightSound;
        cmd.action = [volume](xy::Entity ent, float)
        {
            ent.getComponent<xy::AudioEmitter>().setVolume(ent.getComponent<float>() * volume);
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

        /*cmd.targetFlags = CommandID::BasicShadow;
        cmd.action = [volume](xy::Entity ent, float)
        {
            sf::Uint8 alpha = static_cast<sf::Uint8>((1.f - volume) * 255.f);
            ent.getComponent<xy::Sprite>().setColour({ 255,255,255,alpha });
        };
        getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);*/
    }

    //sun direction
    m_sunDirection.x = -4.f * m_waveTable[m_currentIndex];
    m_sunDirection.y = ((m_waveTable[m_currentIndexTwoFold] + interpOffset) + 1.f);

    m_groundShader->setUniform("u_sunDirection", m_sunDirection);

    //interp storm amount
    if (m_stormAmount < m_targetStormAmount)
    {
        m_stormAmount = std::min(m_stormAmount + (dt / StormTransitionTime), m_targetStormAmount);
    }
    else if (m_stormAmount > m_targetStormAmount)
    {
        m_stormAmount = std::max(m_stormAmount - (dt / StormTransitionTime), 0.f);
    }
    auto alpha = static_cast<sf::Uint8>(255.f * m_stormAmount);
    sf::Color cloudColour(180, 180, 180, alpha);
    m_cloudSprite.setColor(cloudColour);

    xy::Command cmd;
    cmd.targetFlags = CommandID::Rain;
    cmd.action = [alpha](xy::Entity e, float)
    {
        e.getComponent<xy::Sprite>().setColour({ 255, 255,255, alpha });
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::RainSound;
    cmd.action = [&](xy::Entity e, float)
    {
        e.getComponent<xy::AudioEmitter>().setVolume(m_stormAmount * 0.11f);
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    cmd.targetFlags = CommandID::WindSound;
    cmd.action = [&](xy::Entity e, float)
    {
        auto& audio = e.getComponent<xy::AudioEmitter>();

        if (m_doLightning || //lightning might be disabled before fading out
            ((m_stormAmount > m_targetStormAmount) && audio.getVolume() > 0))
        {
            e.getComponent<xy::AudioEmitter>().setVolume(m_stormAmount * 0.41f);
        }
    };
    getScene()->getSystem<xy::CommandSystem>().sendCommand(cmd);

    //PRINT("Storm amount", std::to_string(m_stormAmount));

    //sun colour
    auto currColour = lerp(skyColours[0], skyColours[1], m_sunDirection.y / 2.f);
    currColour = HSVtoRGB(currColour);
    currColour *= (WhiteColour - (StormColour * m_stormAmount));


    //interp flare amount
    if (m_flareAmount < m_targetFlareAmount)
    {
        m_flareAmount = std::min(m_flareAmount + (dt / FlareTransitionTime), m_targetFlareAmount);

        //shrink again immediately
        if (m_flareAmount >= m_targetFlareAmount)
        {
            m_targetFlareAmount = 0.f;
        }
    }
    else if (m_flareAmount > m_targetFlareAmount)
    {
        m_flareAmount = std::max(m_flareAmount - (dt / FlareTransitionTime), 0.f);
    }

    float flareAmount = (m_isDayTime) ? m_flareAmount / 2.f : m_flareAmount;

    currColour += ((FlareColour * m_flareFlickerTable[m_flickerIndex]) * flareAmount);
    m_flickerIndex = (m_flickerIndex + 1) % m_flareFlickerTable.size();

    if (m_doLightning && updateLightning())
    {
        currColour = { 2.f, 2.f, 2.f, 1.f };
    }

    for (auto s : m_shaders)
    {
        s->setUniform("u_skyColour", currColour);
    }


    //update sky sprite
    static float timeAccumulator = 0.f;
    timeAccumulator += dt;

    auto c = static_cast<sf::Uint8>((1.f - (m_sunDirection.y / 2.f)) * 255.f);

    m_bufferSprite.setPosition(getScene()->getActiveCamera().getComponent<xy::Transform>().getPosition());
    m_skyShader->setUniform("u_time", timeAccumulator * 0.005f);

    auto starColour = sf::Color::White;
    starColour.a = 255 - c;
    m_starSprite.move(0.f, m_waveTable[m_currentIndexTwoFold] * 2.f * dt);
    m_starSprite.setColor(starColour);

    //and moon / sun
    float movement = 7.f;
    static const float verticalTravel = xy::DefaultSceneSize.y / 2.f;

    float interp = (verticalTravel * m_waveTable[nextIndex]) - (verticalTravel * m_waveTable[m_currentIndexTwoFold]);
    interp *= m_dayTimer.getElapsedTime().asSeconds(); //this should reset @ 1sec

    if (m_currentIndexTwoFold < m_waveTable.size() / 2)
    {
        movement = -movement;
    }
    m_sunSprite.move(movement * dt, 0.f);
    
    sf::Color sunColour(255, c, c);
    m_sunSprite.setPosition(m_sunSprite.getPosition().x, (verticalTravel * m_waveTable[m_currentIndexTwoFold] + interp) + verticalTravel);
    m_sunSprite.setColor(sunColour);

    m_moonSprite.move(-movement * dt, 0.f);
    m_moonSprite.setPosition(m_moonSprite.getPosition().x, (verticalTravel - m_sunSprite.getPosition().y) + verticalTravel);
    m_moonShader->setUniform("u_dayNumber", m_dayNumber + (static_cast<float>(m_currentIndexTwoFold) / static_cast<float>(m_waveTable.size())));


    //update these more slowly for a longer cycle
    if (m_dayTimer.getElapsedTime().asMilliseconds() > 1000)
    {
        m_currentIndex = (m_currentIndex + 1) % m_waveTable.size();
        m_currentIndexTwoFold = (m_currentIndexTwoFold + 2) % m_waveTable.size();
        m_dayTimer.restart();

        if (m_currentIndexTwoFold == 0)
        {
            m_dayNumber++;
            //std::cout << "Day number " << m_dayNumber << "\n";
        }
    }
}

void DayNightSystem::setStormLevel(std::int32_t level)
{
    switch (level)
    {
    default:
    case 0:
        m_targetStormAmount = 0.f;
        m_doLightning = false;
        m_lightning.reset();
        getScene()->getSystem<FoliageSystem>().setWindStrength(0.06f);
        getScene()->getSystem<FlappySailSystem>().setWindStrength(0.f);
        break;
    case 1:
        m_targetStormAmount = 1.f;
        m_doLightning = false;
        m_lightning.reset();
        getScene()->getSystem<FoliageSystem>().setWindStrength(0.2f);
        getScene()->getSystem<FlappySailSystem>().setWindStrength(8.f);
        break;
    case 2:
        m_targetStormAmount = 1.f;
        m_doLightning = true;
        m_lightning.reset();

        auto* msg = postMessage<MapEvent>(MessageID::MapMessage);
        msg->type = MapEvent::LightningFlash;
        msg->position = Global::IslandSize / 2.f;
        msg->value = 0.15f; //audio volume

        getScene()->getSystem<FoliageSystem>().setWindStrength(0.4f);
        getScene()->getSystem<FlappySailSystem>().setWindStrength(20.f);
        break;
    }
}

//private
bool DayNightSystem::updateLightning()
{
    float lightningTime = m_lightning.timer.getElapsedTime().asSeconds();

    if (lightningTime > m_lightning.times[m_lightning.flashIndex])
    {
        m_lightning.timer.restart();
        m_lightning.flashIndex = (m_lightning.flashIndex + 1) % m_lightning.times.size();
        m_lightning.visible = !m_lightning.visible;

        if (m_lightning.visible && lightningTime > 2.f)
        {
            auto* msg = postMessage<MapEvent>(MessageID::MapMessage);
            msg->type = MapEvent::LightningFlash;
            msg->position = Global::IslandSize / 2.f;
            msg->value = 0.15f; //used as volume
        }


    }

    return m_lightning.visible;
}

void DayNightSystem::setTimeOfDay(float tod)
{
    m_currentIndex = static_cast<std::size_t>(static_cast<float>(m_waveTable.size() * tod));
    m_currentIndexTwoFold = m_currentIndex * 2;
    m_currentIndexTwoFold %= m_waveTable.size();
}

void DayNightSystem::draw(sf::RenderTarget& rt, sf::RenderStates) const
{
    sf::RenderStates states;
    
    m_bufferTexture.clear(sf::Color::Blue);

    states.shader = m_skyShader;
    m_skyShader->setUniform("u_texture", *m_skySprite.getTexture());
    m_bufferTexture.draw(m_skySprite, states);

    states.shader = nullptr;
    m_bufferTexture.draw(m_starSprite, states);
    m_bufferTexture.draw(m_sunSprite, states);

    states.shader = m_moonShader;
    m_bufferTexture.draw(m_moonSprite, states);

    m_skyShader->setUniform("u_texture", *m_cloudSprite.getTexture());
    m_bufferTexture.draw(m_cloudSprite, m_skyShader);

    m_bufferTexture.display();

    states.transform = m_cameraTransform;
    rt.draw(m_bufferSprite/*, states*/);
}
