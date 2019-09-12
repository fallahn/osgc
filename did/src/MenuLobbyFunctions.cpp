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
    size = std::min(Global::MaxNameSize, size - sizeof(std::uint8_t));

    std::uint8_t playerID = 0;
    std::memcpy(&playerID, evt.packet.getData(), sizeof(playerID));

    std::vector<sf::Uint32> buffer(size / sizeof(sf::Uint32));
    std::memcpy(buffer.data(), (char*)evt.packet.getData() + sizeof(playerID), size);

    m_sharedData.clientInformation.getClient(playerID).name = sf::String::fromUtf32(buffer.begin(), buffer.end());

    //TODO update avatar to show human player
}