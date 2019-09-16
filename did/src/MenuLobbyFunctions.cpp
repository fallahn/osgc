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

#include <SFML/System/String.hpp>

namespace
{
    const sf::Vector2f ItemStartPosition(202.f, 188.f);
    const float ItemVerticalSpacing = 76.f;

    std::array<sf::Vector2f, 4u> HostPositions =
    {
        sf::Vector2f(255.f, 280.f),
        {725.f, 280.f},
        {1195.f, 280.f},
        {1665.f, 280.f}
    };
}

void MenuState::sendPlayerData()
{
    auto nameBytes = m_sharedData.clientName.toUtf32();
    auto size = std::min(nameBytes.size() * sizeof(sf::Uint32), Global::MaxNameSize);

    m_sharedData.netClient->sendPacket(PacketID::SupPlayerInfo, nameBytes.data(), size, xy::NetFlag::Reliable, Global::ReliableChannel);
}

void MenuState::updateClientInfo(const xy::NetEvent& evt)
{
    auto size = evt.packet.getSize();
    size = std::min(Global::MaxNameSize, size - (sizeof(std::uint8_t) + sizeof(std::uint64_t)));

    std::uint8_t playerID = 0;
    std::memcpy(&playerID, evt.packet.getData(), sizeof(playerID));

    std::uint64_t peerID = 0;
    std::memcpy(&peerID, (char*)evt.packet.getData() + sizeof(playerID), sizeof(peerID));

    std::vector<sf::Uint32> buffer(size / sizeof(sf::Uint32));
    std::memcpy(buffer.data(), (char*)evt.packet.getData() + sizeof(playerID) + sizeof(peerID), size);

    m_sharedData.clientInformation.getClient(playerID).name = sf::String::fromUtf32(buffer.begin(), buffer.end());
    m_sharedData.clientInformation.getClient(playerID).peerID = peerID;
}

void MenuState::updateHostInfo(std::uint64_t hostID)
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
    }
}