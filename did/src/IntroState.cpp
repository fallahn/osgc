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

#ifdef STAND_ALONE

#include "IntroState.hpp"

#include <xyginext/core/App.hpp>
#include <xyginext/core/FileSystem.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/Window/Event.hpp>

namespace
{
    const float fadeTime = 1.f;
    const float holdTime = 3.5f;

    float accumulatedTime = 0.f;
    const float scanlineCount = 6500.f;

    const std::string scanLineFragment =
        "#version 120\n" \

        "uniform sampler2D u_sourceTexture;\n" \

        "void main()\n" \
        "{\n" \
        "    vec3 colour = texture2D(u_sourceTexture, gl_TexCoord[0].xy).rgb;\n" \
        "    if(mod(floor(gl_FragCoord.y), 2) == 0) colour *= 0.5;\n" \
        "    gl_FragColor = vec4(colour, 0.5);\n" \
        "}";

    const std::string cromFragment =
        "#version 120\n" \
        
        "uniform sampler2D u_sourceTexture;\n" \
        "uniform float u_time;\n" \
        "uniform float u_lineCount = 6000.0;\n" \

        "const float noiseStrength = 0.7;\n" \
        "const float maxOffset = 1.0 / 450.0;\n" \

        "const float centreDistanceSquared = 0.25;\n" \
        "float distanceSquared(vec2 coord)\n" \
        "{\n" \
        "    return dot(coord, coord);\n" \
        "}\n" \

        "void main()\n" \
        "{\n" \
        "    vec2 texCoord = gl_TexCoord[0].xy;\n" \

        "    vec2 offset = vec2((maxOffset / 2.0) - (texCoord.x * maxOffset), (maxOffset / 2.0) - (texCoord.y * maxOffset));\n"
        "    vec3 colour = vec3(0.0);\n" \

        "    colour.r = texture2D(u_sourceTexture, texCoord + offset).r;\n" \
        "    colour.g = texture2D(u_sourceTexture, texCoord).g;\n" \
        "    colour.b = texture2D(u_sourceTexture, texCoord - offset).b;\n" \

        /*noise*/
        "    float x = (texCoord.x + 4.0) * texCoord.y * u_time * 10.0;\n" \
        "    x = mod(x, 13.0) * mod(x, 123.0);\n" \
        "    float grain = mod(x, 0.01) - 0.005;\n" \
        "    vec3 result = colour + vec3(clamp(grain * 100.0, 0.0, 0.07));\n" \
        /*scanlines*/
        "    vec2 sinCos = vec2(sin(texCoord.y * u_lineCount), cos(texCoord.y * u_lineCount + u_time));\n" \
        "    result += colour * vec3(sinCos.x, sinCos.y, sinCos.x) * (noiseStrength * 0.08);\n" \
        "    colour += (result - colour) * noiseStrength;\n" \
        "    gl_FragColor = vec4(colour, 1.0);" \
        "}";
}

IntroState::IntroState(xy::StateStack& stack, Context context)
    : xy::State(stack, context),
    m_windowRatio(static_cast<float>(context.renderWindow.getSize().y) / context.defaultView.getSize().y),
    m_messageBus(context.appInstance.getMessageBus()),
    m_fadeTime(0.f),
    m_fade(Fade::In)
{
    m_texture.loadFromFile(xy::FileSystem::getResourcePath() + "assets/images/startup.png");
    m_sprite.setTexture(m_texture);
    m_rectangleShape.setSize(xy::DefaultSceneSize);

    m_noiseShader.loadFromMemory(cromFragment, sf::Shader::Fragment);
    m_noiseShader.setUniform("u_lineCount", m_windowRatio * scanlineCount);
    m_noiseShader.setUniform("u_sourceTexture", m_texture);

    m_lineShader.loadFromMemory(scanLineFragment, sf::Shader::Fragment);
    m_lineShader.setUniform("u_sourceTexture", m_texture);

    m_soundBuffer.loadFromFile(xy::FileSystem::getResourcePath() + "assets/sound/startup.wav");
    m_sound.setBuffer(m_soundBuffer);
    m_sound.setVolume(5.f);
    m_sound.play();

    xy::App::setMouseCursorVisible(false);
    context.renderWindow.setView(context.defaultView);
}

//public
bool IntroState::handleEvent(const sf::Event & evt)
{
    if (evt.type == sf::Event::KeyReleased
        || evt.type == sf::Event::JoystickButtonReleased
        || evt.type == sf::Event::MouseButtonReleased)
    {
        requestStackPop();
        requestStackPush(StateID::Menu);
    }
    return false;
}

void IntroState::handleMessage(const xy::Message& msg)
{
    if (msg.id == xy::Message::WindowMessage)
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
            auto context = getContext();
            context.renderWindow.setView(context.defaultView);
        }
    }
}

bool IntroState::update(float dt)
{
    accumulatedTime += dt;
    m_noiseShader.setUniform("u_time", accumulatedTime * (10.f * m_windowRatio));

    sf::Color colour = sf::Color::Black;
    m_fadeTime += dt;
    switch (m_fade)
    {
    case Fade::In:
        colour.a = static_cast<sf::Uint8>(std::min(1.f - ((m_fadeTime / fadeTime) * 255.f), 0.f));
        if (m_fadeTime >= fadeTime)
        {
            m_fade = Fade::Hold;
            m_fadeTime = 0.f;
        }
        break;
    case Fade::Hold:
        colour.a = 0u;
        if (m_fadeTime >= holdTime)
        {
            m_fadeTime = 0.f;
            m_fade = Fade::Out;
        }
        break;
    case Fade::Out:
        colour.a = static_cast<sf::Uint8>(std::min((m_fadeTime / fadeTime) * 255.f, 255.f));
        if (m_fadeTime >= fadeTime)
        {
            requestStackPop();
            requestStackPush(StateID::Menu);
        }
        break;
    default: break;
    }
    m_rectangleShape.setFillColor(colour);
    return false;
}

void IntroState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_sprite, &m_noiseShader);

    if (xy::Util::Random::value(0, 10) == 0)
    {
        m_sprite.setPosition(9.f, -7.f);
        rw.draw(m_sprite, &m_lineShader);
        m_sprite.setPosition(0.f, 0.f);
    }

    rw.draw(m_rectangleShape);
}

#endif