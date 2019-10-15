#include "MenuState.hpp"
#include "MessageIDs.hpp"
#include "SharedStateData.hpp"
#include "NetworkClient.hpp"
#include "ClientInfoManager.hpp"
#include "PacketTypes.hpp"
#include "GlobalConsts.hpp"
#include "Packet.hpp"

#include <xyginext/core/Log.hpp>
#include <xyginext/resources/Resource.hpp>

#include <xyginext/ecs/components/Transform.hpp>
#include <xyginext/ecs/components/Drawable.hpp>
#include <xyginext/ecs/components/Sprite.hpp>
#include <xyginext/ecs/components/Text.hpp>
#include <xyginext/ecs/components/CommandTarget.hpp>
#include <xyginext/ecs/components/Callback.hpp>
#include <xyginext/ecs/components/UIHitBox.hpp>
#include <xyginext/util/Random.hpp>

#include <SFML/System/String.hpp>

namespace
{
    const sf::Vector2f ItemStartPosition(202.f, 188.f);

    std::array<sf::Vector2f, 4u> HostPositions =
    {
        sf::Vector2f(262.f, 230.f),
        {732.f, 230.f},
        {1202.f, 230.f},
        {1672.f, 230.f}
    };
}

void MenuState::sendPlayerData()
{
    auto nameBytes = m_sharedData.clientName.toUtf32();
    auto size = std::min(nameBytes.size() * sizeof(sf::Uint32), Global::MaxNameSize);

    std::vector<std::uint8_t>  buffer(size + sizeof(PlayerInfoHeader));
    PlayerInfoHeader ph;
    ph.spriteIndex = m_spriteIndex;
    ph.hatIndex = m_hatIndex;
    
    std::memcpy(buffer.data(), &ph, sizeof(ph));
    std::memcpy(buffer.data() + sizeof(ph), nameBytes.data(), size);

    m_sharedData.netClient->sendPacket(PacketID::SupPlayerInfo, buffer.data(), buffer.size(), xy::NetFlag::Reliable, Global::ReliableChannel);
}

void MenuState::updateClientInfo(const xy::NetEvent& evt)
{
    auto size = evt.packet.getSize();
    size = std::min(Global::MaxNameSize, size - sizeof(ClientInfoHeader));

    ClientInfoHeader ch;
    std::memcpy(&ch, evt.packet.getData(), sizeof(ch));

    std::vector<sf::Uint32> buffer(size / sizeof(sf::Uint32));
    std::memcpy(buffer.data(), (char*)evt.packet.getData() + sizeof(ch), size);

    auto& client = m_sharedData.clientInformation.getClient(ch.playerID);
    bool newPlayer = client.peerID != ch.peerID; //this client is not yet known to us!
    client.name = sf::String::fromUtf32(buffer.begin(), buffer.end());
    client.peerID = ch.peerID;
    client.spriteIndex = ch.spriteIndex;
    client.hatIndex = (ch.hatIndex < m_hatTextureIDs.size()) ? ch.hatIndex : 0;

    //TODO if we're updating the character sprite immediately, do we need to store the hat index in the client data?

    //print a message
    if (newPlayer)
    {
        m_chatBuffer.push_back(client.name + " joined the game");

        if (m_chatBuffer.size() > MaxChatSize)
        {
            m_chatBuffer.pop_front();
        }

        //update the sprite texture
        updateHatTexture(m_resources.get<sf::Texture>(m_playerTextureIDs[ch.playerID]), m_resources.get<sf::Texture>(m_hatTextureIDs[client.hatIndex]), *m_sharedData.playerSprites[ch.playerID]);

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::ChatOutText;
        cmd.action = std::bind(&MenuState::updateChatOutput, this, std::placeholders::_1, std::placeholders::_2);
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void MenuState::updateHostInfo(std::uint64_t hostID, std::string remoteAddress)
{
    m_sharedData.clientInformation.setHostID(hostID);

    {
        sf::Vector2f position = (m_sharedData.clientInformation.getHostID() == m_sharedData.netClient->getPeer().getID()) ?
            Menu::StartButtonPosition : Menu::StartButtonPositionHidden;

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::StartButton;
        cmd.action = [position](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(position);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        //update the host icon - although this will nearly always be 0
        for (auto i = 0; i < 4; ++i)
        {
            if (m_sharedData.clientInformation.getClient(i).peerID == hostID)
            {
                cmd.targetFlags = Menu::CommandID::HostIcon;
                cmd.action = [i](xy::Entity e, float)
                {
                    e.getComponent<xy::Transform>().setPosition(HostPositions[i]);
                };
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

                break;
            }
        }

        //display our local IP just for info's sake        
        if (hostID == m_sharedData.netClient->getPeer().getID())
        {
            auto& chatFont = m_resources.get<sf::Font>(m_fontIDs[Menu::FontID::Chat]);
            
            auto entity = m_uiScene.createEntity();
            entity.addComponent<xy::Transform>().setPosition(12.f, -6.f);
            entity.addComponent<xy::Drawable>().setDepth(Menu::SpriteDepth::Near);
            entity.addComponent<xy::Text>(chatFont).setFillColour(sf::Color::White);
            entity.getComponent<xy::Text>().setCharacterSize(32);
            entity.getComponent<xy::Text>().setString("Hosting on " + remoteAddress + ":" + std::to_string(Global::GamePort));
            entity.addComponent<xy::Callback>().active = true;
            entity.getComponent<xy::Callback>().function =
                [&](xy::Entity e, float)
            {
                if (!m_sharedData.gameServer->running())
                {
                    m_uiScene.destroyEntity(e);
                }
            };

            cmd.targetFlags = Menu::CommandID::LobbyNode;
            cmd.action = [entity](xy::Entity parentEntity, float) mutable
            {
                parentEntity.getComponent<xy::Transform>().addChild(entity.getComponent<xy::Transform>());
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }
    }
}

void MenuState::applySeed()
{
    if (m_activeString == &m_seedDisplayString)
    {
        if (m_activeString->isEmpty())
        {
            //don't have empty seeds
            randomiseSeed();

            auto* seedString = m_activeString;

            xy::Command cmd;
            cmd.targetFlags = Menu::CommandID::SeedText;
            cmd.action = [seedString](xy::Entity entity, float)
            {
                auto& text = entity.getComponent<xy::Text>();
                text.setString(*seedString);
            };
            m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
        }

        Server::SeedData sd;
        std::strcpy(sd.str, m_seedDisplayString.toAnsiString().c_str());
        m_sharedData.netClient->sendPacket(PacketID::SetSeed, sd, xy::NetFlag::Reliable);

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::SeedText;
        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setFillColour(sf::Color::White); };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void MenuState::randomiseSeed()
{
    m_activeString = &m_seedDisplayString;
    auto length = xy::Util::Random::value(4, 8);
    auto string = std::to_string(xy::Util::Random::value(static_cast<int>(std::pow(10, length - 1)), std::pow(10, length)));

    auto passes = xy::Util::Random::value(0, 10);
    for (auto i = 0; i < passes; ++i)
    {
        //add a random char
        string[xy::Util::Random::value(0, string.size() - 1)] = static_cast<char>(xy::Util::Random::value(32, 126));
    }
    std::shuffle(string.begin(), string.end(), xy::Util::Random::rndEngine);
    *m_activeString = string;
}

void MenuState::sendChat()
{
    if (m_activeString == &m_chatInDisplayString
        && !m_chatInDisplayString.isEmpty())
    {
        auto stringBytes = m_chatInDisplayString.toUtf32();
        std::vector<std::uint8_t> buffer((stringBytes.size() * sizeof(sf::Uint32)) + (2 * sizeof(std::uint8_t)));
        buffer[0] = m_sharedData.clientInformation.getClientFromPeer(m_sharedData.netClient->getPeer().getID());
        buffer[1] = static_cast<std::uint8_t>(stringBytes.size() * sizeof(sf::Uint32));

        std::memcpy(&buffer[2], stringBytes.data(), stringBytes.size() * sizeof(sf::Uint32));

        if (buffer[0] != 255)
        {
            m_sharedData.netClient->sendPacket(PacketID::ChatMessage, buffer.data(), buffer.size(), xy::NetFlag::Reliable);
        }

        m_chatInDisplayString.clear();

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::ChatInText;
        cmd.action = [](xy::Entity e, float) {e.getComponent<xy::Text>().setString(""); };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void MenuState::receiveChat(const xy::NetEvent& evt)
{
    if (evt.packet.getSize() > 2)
    {
        auto dataSize = std::min(evt.packet.getSize(), (Menu::MaxChatChar) * sizeof(sf::Uint32) + 2);

        std::vector<std::uint8_t> buffer(dataSize);
        std::memcpy(buffer.data(), evt.packet.getData(), dataSize);

        auto playerID = buffer[0];
        auto strSize = buffer[1];

        sf::String chatString;

        if (strSize >= sizeof(sf::Uint32))
        {
            std::vector<sf::Uint32> stringBuf(strSize / sizeof(sf::Uint32));
            std::memcpy(stringBuf.data(), &buffer[2], strSize);
            chatString = sf::String::fromUtf32(stringBuf.begin(), stringBuf.end());

            if (playerID < 4)
            {
                m_chatBuffer.push_back(m_sharedData.clientInformation.getClient(playerID).name + ": " + chatString);
                if (m_chatBuffer.size() > MaxChatSize)
                {
                    m_chatBuffer.pop_front();
                }

                xy::Command cmd;
                cmd.targetFlags = Menu::CommandID::ChatOutText;
                cmd.action = std::bind(&MenuState::updateChatOutput, this, std::placeholders::_1, std::placeholders::_2);
                m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
            }
        }
    }
    else
    {
        xy::Logger::log("Chat message packet too small", xy::Logger::Type::Error);
    }
}

void MenuState::updateChatOutput(xy::Entity e, float)
{
    sf::String outStr;
    for (const auto& str : m_chatBuffer)
    {
        outStr += str + "\n";
    }
    e.getComponent<xy::Text>().setString(outStr);
    e.getComponent<xy::AudioEmitter>().play();
}