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

#include "TorchlightSystem.hpp"
#include "GlobalConsts.hpp"
#include "MessageIDs.hpp"
#include "ResourceIDs.hpp"
#include "glad/glad.h"
#include "fastnoise/FastNoiseSIMD.h"
using fn = FastNoiseSIMD;

#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/resources/ShaderResource.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Graphics/Shader.hpp>

#include <array>

namespace
{
    const std::size_t MaxTorches = 4;
    std::array<std::string, MaxTorches> ColourUniforms =
    {
        "u_lightColour[0]", "u_lightColour[1]", "u_lightColour[2]", "u_lightColour[3]"
    };

    std::array<std::string, MaxTorches> PositionUniforms =
    {
        "u_lightWorldPosition[0]", "u_lightWorldPosition[1]", "u_lightWorldPosition[2]", "u_lightWorldPosition[3]"
    };

    static const int NoiseTableSize = 128;
    static const int NoiseOffset = NoiseTableSize / MaxTorches;
}

TorchlightSystem::TorchlightSystem(xy::MessageBus& mb, xy::ShaderResource& shaders)
    : xy::System    (mb, typeid(TorchlightSystem)),
    m_dayTime       (Global::DayCycleOffset),
    m_noiseIndex    (0),
    m_prepCount     (2)
{
    //add shaders to internal list
    m_shaders.push_back(&shaders.get(ShaderID::SpriteShader));
    m_shaders.push_back(&shaders.get(ShaderID::SpriteShaderCulled));
    m_shaders.push_back(&shaders.get(ShaderID::LandShader));
    m_shaders.push_back(&shaders.get(ShaderID::PlaneShader));

    requireComponent<Torchlight>();
    requireComponent<xy::Transform>();

    auto noise = fn::NewFastNoiseSIMD();
    noise->SetFractalOctaves(5);
    noise->SetFrequency(0.08f);
    noise->SetFractalGain(0.1f);

    auto lightNoise = noise->GetSimplexFractalSet(0, 0, 0, NoiseTableSize, 1, 1);
    for (auto i = 0u; i < NoiseTableSize; ++i)
    {
        m_noiseTable.push_back((lightNoise[i] + 1.f) /** 0.5f*/);
        //m_noiseTable.back() *= 0.082f;
    }
    fn::FreeNoiseSet(lightNoise);
}

//public
void TorchlightSystem::handleMessage(const xy::Message& msg)
{
    //handle server day time updates
    switch (msg.id)
    {
    default: break;
    case MessageID::MapMessage:
    {
        const auto& data = msg.getData<MapEvent>();
        if (data.type == MapEvent::DayNightUpdate)
        {
            m_dayTime = data.value;
        }
    }
    break;
    }
}

void TorchlightSystem::process(float dt)
{
    prepShaders();

    //interpolate day time
    m_dayTime += dt / Global::DayNightSeconds;

    //for each entity update colour and set shader property (colour / world pos)
    auto& entities = getEntities();
    for (auto i = 0u; i < MaxTorches && i < entities.size(); ++i)
    {
        auto idx = (m_noiseIndex + (i * NoiseOffset)) % NoiseTableSize;

        auto& torch = entities[i].getComponent<Torchlight>();
        torch.colour = { 0.25f, 0.018f + (m_noiseTable[idx] * 0.08f), m_noiseTable[idx] * 0.08f };

        auto pos = entities[i].getComponent<xy::Transform>().getWorldPosition();
        sf::Glsl::Vec3 worldPosition = { pos.x, Torchlight::height, pos.y + 10.f };

        /*for (auto shader : m_shaders)
        {
            shader->setUniform(ColourUniforms[i], torch.colour);
            shader->setUniform(PositionUniforms[i], worldPosition);
        }*/

        for (const auto& uniform : m_uniforms)
        {
            glUseProgram(uniform.colours[i].first);
            glUniform3f(uniform.colours[i].second, torch.colour.x, torch.colour.y, torch.colour.z);

            glUseProgram(uniform.positions[i].first);
            glUniform3f(uniform.positions[i].second, worldPosition.x, worldPosition.y, worldPosition.z);
        }
        glUseProgram(0);

        /*sf::Uint8 c = static_cast<sf::Uint8>(255.f * m_noiseTable[idx]);
        entities[i].getComponent<xy::Sprite>().setColour({ c,c,c });*/
    }

    m_noiseIndex = (m_noiseIndex + 1) % NoiseTableSize;
}

//private
void TorchlightSystem::prepShaders()
{
    if (m_prepCount > 0)
    {
        m_uniforms.clear();
        m_prepCount--;

        for (auto s : m_shaders)
        {
            auto handle = s->getNativeHandle();
            glUseProgram(handle);

            UniformLocations locations;

            for (auto i = 0; i < ColourUniforms.size(); ++i)
            {
                auto loc = glGetUniformLocation(handle, ColourUniforms[i].c_str());
                locations.colours[i] = std::make_pair(handle, loc);

                loc = glGetUniformLocation(handle, PositionUniforms[i].c_str());
                locations.positions[i] = std::make_pair(handle, loc);
            }
            m_uniforms.push_back(locations);
        }

        glUseProgram(0);
    }
}