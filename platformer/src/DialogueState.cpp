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

#include "DialogueState.hpp"
#include "SharedStateData.hpp"
#include "GameConsts.hpp"
#include "ResourceIDs.hpp"

#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Camera.hpp>
#include <xyginext/ecs/components/Callback.hpp>

#include <xyginext/ecs/systems/TextSystem.hpp>
#include <xyginext/ecs/systems/RenderSystem.hpp>
#include <xyginext/ecs/systems/CallbackSystem.hpp>

#include <xyginext/gui/Gui.hpp>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Clock.hpp>

#include <fstream>

namespace
{
    sf::Clock delayClock;
    const sf::Time delayTime = sf::seconds(2.f);

    struct LineData final
    {
        std::size_t lineIndex = 0;
        std::size_t charIndex = 0;
        float charTime = 0.f;
        static constexpr float TimePerChar = 0.05f;
    };

    struct LineCallback final
    {
        explicit LineCallback(const std::vector<std::string>& lines)
            : m_lines(lines) {}

        void operator () (xy::Entity entity, float dt)
        {
            auto& lineData = entity.getComponent<LineData>();
            const auto& str = m_lines[lineData.lineIndex];

            if (lineData.charIndex < str.size())
            {
                lineData.charTime += dt;
                if (lineData.charTime > LineData::TimePerChar)
                {
                    lineData.charIndex++;
                    lineData.charTime = 0.f;
                }
            }
            else
            {
                entity.getComponent<xy::Callback>().active = false;
            }
            entity.getComponent<xy::Text>().setString(str.substr(0, lineData.charIndex));
        }

    private:
        const std::vector<std::string>& m_lines;
    };
}

DialogueState::DialogueState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State (ss, ctx),
    m_scene     (ctx.appInstance.getMessageBus()),
    m_sharedData(sd),
    m_lineIndex (0)
{
    build();

    m_scene.getActiveCamera().getComponent<xy::Camera>().setView(ctx.defaultView.getSize());
    m_scene.getActiveCamera().getComponent<xy::Camera>().setViewport(ctx.defaultView.getViewport());
}

//public
bool DialogueState::handleEvent(const sf::Event& evt)
{
    //prevents events being forwarded if the console wishes to consume them
    if (xy::Nim::wantsKeyboard() || xy::Nim::wantsMouse())
    {
        return false;
    }

    //slight delay before accepting input
    if (delayClock.getElapsedTime() > delayTime)
    {
        if (evt.type == sf::Event::KeyReleased)
        {
            displayNextLine();
        }
        else if (evt.type == sf::Event::JoystickButtonReleased
            && evt.joystickButton.joystickId == 0)
        {
            if (evt.joystickButton.button == 0)
            {
                displayNextLine();
            }
        }
    }

    m_scene.forwardEvent(evt);

    return false;
}

void DialogueState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool DialogueState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void DialogueState::draw()
{
    auto& rw = getContext().renderWindow;
    rw.draw(m_scene);
}

//private
void DialogueState::build()
{
    auto& mb = getContext().appInstance.getMessageBus();
    m_scene.addSystem<xy::CallbackSystem>(mb);
    m_scene.addSystem<xy::TextSystem>(mb);
    m_scene.addSystem<xy::RenderSystem>(mb);

    static const std::size_t MaxLines = 12;

    //load text file and replace with missing text message if fails
    auto path = xy::FileSystem::getResourcePath() + "assets/dialogue/" + m_sharedData.dialogueFile;
    std::ifstream file(path);
    if (file.is_open() && file.good())
    {
        std::string line;
        while (m_lines.size() < MaxLines && std::getline(file, line))
        {
            m_lines.push_back(line);
        }
    }
    else
    {
        m_lines.push_back("I'm sorry Dave, I can't do that.");
    }

    if (m_lines.empty())
    {
        m_lines.push_back("I guess there was just nothing to say...");
    }

    createBackground();


    auto fontID = m_resources.load<sf::Font>(FontID::GearBoyFont);
    auto& font = m_resources.get<sf::Font>(fontID);

    auto createText = [&]()->xy::Entity
    {
        auto entity = m_scene.createEntity();
        
        entity.addComponent<xy::Drawable>().setDepth(GameConst::TextDepth);
        entity.addComponent<xy::Text>(font).setCharacterSize(GameConst::UI::SmallTextSize);
        entity.getComponent<xy::Text>().setFillColour(sf::Color::Black);

        entity.addComponent<LineData>();
        entity.addComponent<xy::Callback>().active = true;
        entity.getComponent<xy::Callback>().function = LineCallback(m_lines);

        return entity;
    };
    
    auto entity = createText();
    entity.addComponent<xy::Transform>().setPosition(80.f, xy::DefaultSceneSize.y - 200.f); //TODO make this relative to background
    m_lineEntities[0] = entity;

    entity = createText();
    entity.addComponent<xy::Transform>().setPosition(80.f, xy::DefaultSceneSize.y - 140.f);
    entity.getComponent<xy::Callback>().active = false;
    m_lineEntities[1] = entity;
}

void DialogueState::createBackground()
{
    auto background = m_resources.load<sf::Texture>("assets/images/" + m_sharedData.theme + "/dialogue_background.png");
    m_resources.get<sf::Texture>(background).setRepeated(true);

    const float scale = 4.f; //kludge.

    auto entity = m_scene.createEntity();
    entity.addComponent<xy::Transform>().setScale(scale, scale);
    entity.addComponent<xy::Drawable>().setDepth(GameConst::BackgroundDepth);
    entity.getComponent<xy::Drawable>().setTexture(&m_resources.get<sf::Texture>(background));
    auto& verts = entity.getComponent<xy::Drawable>().getVertices();

    //background is a 9-patch created from a circular image
    sf::Vector2f texSize(entity.getComponent<xy::Drawable>().getTexture()->getSize());
    sf::Vector2f columnSize = texSize / 2.f;
    sf::Vector2f padding(40.f, 40.f); //TODO add to const vals
    sf::Vector2f backgroundSize = xy::DefaultSceneSize - (padding * 2.f);
    backgroundSize.y = 200.f; //TODO constify
    backgroundSize /= scale;

    //top left corner
    verts.emplace_back(sf::Vector2f(), sf::Vector2f());
    verts.emplace_back(sf::Vector2f(columnSize.x, 0.f), sf::Vector2f(columnSize.x, 0.f));
    verts.emplace_back(columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(0.f, columnSize.y), sf::Vector2f(0.f, columnSize.y));
    
    //top middle
    verts.emplace_back(sf::Vector2f(columnSize.x, 0.f), sf::Vector2f(columnSize.x, 0.f));
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, 0.f), sf::Vector2f(columnSize.x, 0.f));
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, columnSize.y), columnSize);
    verts.emplace_back(columnSize, columnSize);

    //top right
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, 0.f), sf::Vector2f(columnSize.x, 0.f));
    verts.emplace_back(sf::Vector2f(backgroundSize.x, 0.f), sf::Vector2f(texSize.x, 0.f));
    verts.emplace_back(sf::Vector2f(backgroundSize.x, columnSize.y), sf::Vector2f(texSize.x, columnSize.y));
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, columnSize.y), columnSize);

    //middle left
    verts.emplace_back(sf::Vector2f(0.f, columnSize.y), sf::Vector2f(0.f, columnSize.y));
    verts.emplace_back(columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y - columnSize.y), columnSize);
    verts.emplace_back(sf::Vector2f(0.f, backgroundSize.y - columnSize.y), sf::Vector2f(0.f, columnSize.y));

    //middle middle
    verts.emplace_back(columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, columnSize.y), columnSize);
    verts.emplace_back(backgroundSize - columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y - columnSize.y), columnSize);

    //middle right
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, columnSize.y), columnSize);
    verts.emplace_back(sf::Vector2f(backgroundSize.x, columnSize.y), sf::Vector2f(texSize.x, columnSize.y));
    verts.emplace_back(sf::Vector2f(backgroundSize.x, backgroundSize.y - columnSize.y), sf::Vector2f(texSize.x, columnSize.y));
    verts.emplace_back(sf::Vector2f(backgroundSize - columnSize), columnSize);

    //bottom left
    verts.emplace_back(sf::Vector2f(0.f, backgroundSize.y - columnSize.y), sf::Vector2f(0.f, columnSize.y));
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y - columnSize.y), columnSize);
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y), sf::Vector2f(columnSize.x, texSize.y));
    verts.emplace_back(sf::Vector2f(0.f, backgroundSize.y), sf::Vector2f(0.f, texSize.y));

    //bottom middle
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y - columnSize.y), columnSize);
    verts.emplace_back(backgroundSize - columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, backgroundSize.y), sf::Vector2f(columnSize.x, texSize.y));
    verts.emplace_back(sf::Vector2f(columnSize.x, backgroundSize.y), sf::Vector2f(columnSize.x, texSize.y));

    //bottom right
    verts.emplace_back(backgroundSize - columnSize, columnSize);
    verts.emplace_back(sf::Vector2f(backgroundSize.x, backgroundSize.y - columnSize.y), sf::Vector2f(texSize.x, columnSize.y));
    verts.emplace_back(backgroundSize, texSize);
    verts.emplace_back(sf::Vector2f(backgroundSize.x - columnSize.x, backgroundSize.y), sf::Vector2f(columnSize.x, texSize.y));

    entity.getComponent<xy::Drawable>().updateLocalBounds();

    entity.getComponent<xy::Transform>().setPosition(padding.x, xy::DefaultSceneSize.y - ((backgroundSize.y * scale) + padding.y));
}

void DialogueState::displayNextLine()
{
    auto entityIndex = m_lineIndex % m_lineEntities.size();
    auto& lineData = m_lineEntities[entityIndex].getComponent<LineData>();

    //if line is active skip to the end
    if (lineData.charIndex < m_lines[lineData.lineIndex].size())
    {
        m_lineEntities[entityIndex].getComponent<xy::Callback>().active = false;
        m_lineEntities[entityIndex].getComponent<xy::Text>().setString(m_lines[m_lineIndex]);
        lineData.charIndex = m_lines[m_lineIndex].size();
    }

    //else move to next line
    else if (m_lineIndex < m_lines.size() - 1)
    {
        m_lineIndex++;
        entityIndex = m_lineIndex % m_lineEntities.size();

        auto& data = m_lineEntities[entityIndex].getComponent<LineData>();
        data.charIndex = 0;
        data.lineIndex = m_lineIndex;
        data.charTime = 0.f;

        m_lineEntities[entityIndex].getComponent<xy::Callback>().active = true;

        if (entityIndex == 0)
        {
            //wrapped around so clear other lines
            for (auto e : m_lineEntities)
            {
                e.getComponent<xy::Text>().setString("");
            }
        }
    }
    else
    {
        //all lines displayed
        requestStackPop();
    }
}