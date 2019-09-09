#include "MenuState.hpp"
#include "MessageIDs.hpp"
#include "SharedStateData.hpp"
#include "NetworkClient.hpp"
#include "ClientInfoManager.hpp"
#include "PacketTypes.hpp"
#include "GlobalConsts.hpp"

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

void MenuState::onLobbyEnter(LobbyEnter_t* cb)
{
    //stash the lobby ID so we can query it for things
    m_sharedData.lobbyID = { cb->m_ulSteamIDLobby };
    m_sharedData.host = SteamMatchmaking()->GetLobbyOwner(m_sharedData.lobbyID);

    //refesh the lobby view
    refreshLobbyView();

    //attempt to join the server - if hosting this sometimes happens too soon..
    SteamMatchmaking()->GetLobbyGameServer(m_sharedData.lobbyID, nullptr, nullptr, &m_sharedData.serverID);
    //sendData(PacketID::None, std::uint8_t(0), m_sharedData.serverID, EP2PSend::k_EP2PSendReliable);

    //TODO get the IP address stored in lobby data - for now this is a kludge assuming we connected locally
    m_sharedData.netClient->connect("127.0.0.1", Global::GamePort);
}

void MenuState::onLobbyDataUpdate(LobbyDataUpdate_t* cb)
{
    if (cb->m_ulSteamIDMember == cb->m_ulSteamIDLobby)
    {
        //set the server ID in shared data
        auto oldServer = m_sharedData.serverID;
        SteamMatchmaking()->GetLobbyGameServer(m_sharedData.lobbyID, nullptr, nullptr, &m_sharedData.serverID);

        if (oldServer != m_sharedData.serverID)
        {
            //disconnect and reconnect to new server
            SteamNetworking()->CloseP2PSessionWithUser(oldServer);
            
            //request join server
            //sendData(PacketID::None, std::uint8_t(0), m_sharedData.serverID, EP2PSend::k_EP2PSendReliable);
        }

        //check if we're the new host and rejigger local server
        auto host = SteamMatchmaking()->GetLobbyOwner(m_sharedData.lobbyID);
        if (host != m_sharedData.host &&
            host == SteamUser()->GetSteamID()) //we just took ownership
        {
            //this should do nothing if server already running
            auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
            msg->action = SystemEvent::RequestStartServer;

            m_sharedData.host = host;
            refreshLobbyView();
        }
        else
        {
            
        }
    }
    else //a user's data has changed
    {
        //update the ready state display
        CSteamID memberID(cb->m_ulSteamIDMember);
        std::string readyState = SteamMatchmaking()->GetLobbyMemberData(m_sharedData.lobbyID, memberID, "ready");
        bool isReady = (readyState == "true");

        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::ReadyBox;
        cmd.action = [memberID, isReady](xy::Entity entity, float)
        {
            if (entity.getComponent<CSteamID>() == memberID)
            {
                auto bounds = entity.getComponent<xy::Sprite>().getTextureRect();
                if (isReady)
                {
                    bounds.top = bounds.height;
                }
                else
                {
                    bounds.top = 0.f;
                }
                entity.getComponent<xy::Sprite>().setTextureRect(bounds);
            }
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);
    }
}

void MenuState::onLobbyChatUpdate(LobbyChatUpdate_t* cb)
{
    if (cb->m_rgfChatMemberStateChange & EChatMemberStateChange::k_EChatMemberStateChangeEntered)
    {
        xy::Logger::log(std::to_string(cb->m_ulSteamIDUserChanged) + " has joined the lobby", xy::Logger::Type::Info);
    }
    else
    {
        xy::Logger::log(std::to_string(cb->m_ulSteamIDUserChanged) + " has left the lobby", xy::Logger::Type::Info);
        //TODO we can filter reason via enum
    }
    refreshLobbyView();
}

void MenuState::onChatMessage(LobbyChatMsg_t* cb)
{
    std::array<char, 4096u> buffer;  
    auto msgSize = SteamMatchmaking()->GetLobbyChatEntry(cb->m_ulSteamIDLobby, cb->m_iChatID, nullptr, &buffer[0], buffer.size(), nullptr);

    if (msgSize > 0)
    {
        switch (buffer[0])
        {
        default: break;
        case Menu::ChatMessageID::StartGame:
            //TODO display a 'joining...' message
            break;
        case Menu::ChatMessageID::ChatText:
            //TODO print text in lobby
            break;
        }
    }
}

void MenuState::onLobbyCreated(LobbyCreated_t* cb, bool failed)
{
    if (failed)
    {
        //dump back to main menu and quit server
        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::LobbyNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        
        cmd.targetFlags = Menu::CommandID::MainNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition({});
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        auto* msg = getContext().appInstance.getMessageBus().post<SystemEvent>(MessageID::SystemMessage);
        msg->action = SystemEvent::RequestStopServer;


        xy::Logger::log("Failed to create lobby", xy::Logger::Type::Error);
        return;
    }

    //set server/seed data on lobby
    CSteamID id(cb->m_ulSteamIDLobby);
    SteamMatchmaking()->SetLobbyGameServer(id, 0, 0, m_sharedData.serverID);

    //attempts to join if we're hosting.
    //sendData(PacketID::None, std::uint8_t(0), m_sharedData.serverID, EP2PSend::k_EP2PSendReliable);
    m_sharedData.netClient->connect("127.0.0.1", Global::GamePort);

    //enable pinging in case first attempt fails
    m_pingServer = true;
}

void MenuState::onLobbyJoined(LobbyEnter_t* cb, bool failed)
{
    if (failed || cb->m_EChatRoomEnterResponse != EChatRoomEnterResponse::k_EChatRoomEnterResponseSuccess)
    {
        //return to main menu - TODO output some message why
        xy::Command cmd;
        cmd.targetFlags = Menu::CommandID::MainNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition({});
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = Menu::CommandID::LobbyNode;
        cmd.action = [](xy::Entity e, float)
        {
            e.getComponent<xy::Transform>().setPosition(Menu::OffscreenPosition);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        cmd.targetFlags = Menu::CommandID::LobbyItem;
        cmd.action = [&](xy::Entity entity, float)
        {
            m_uiScene.destroyEntity(entity);
        };
        m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

        if (m_sharedData.lobbyID.IsLobby())
        {
            SteamMatchmaking()->LeaveLobby(m_sharedData.lobbyID);
            m_sharedData.lobbyID = {};
        }
    }
}

void MenuState::refreshLobbyView()
{
    //clear existing
    xy::Command cmd;
    cmd.targetFlags = Menu::CommandID::LobbyItem;
    cmd.action = [&](xy::Entity entity, float)
    {
        m_uiScene.destroyEntity(entity);
    };
    m_uiScene.getSystem<xy::CommandSystem>().sendCommand(cmd);

    auto& font = m_fontResource.get(Global::FineFont);
    auto owner = SteamMatchmaking()->GetLobbyOwner(m_sharedData.lobbyID);
    //this defers setting the ID by a frame so we don't accidentally delete the new item
    auto callback = [](xy::Entity entity, float)
    {
        entity.getComponent<xy::CommandTarget>().ID = Menu::CommandID::LobbyItem;
        entity.getComponent<xy::Callback>().active = false;
    };

    //update member data
    auto memberCount = SteamMatchmaking()->GetNumLobbyMembers(m_sharedData.lobbyID);
    for (auto i = 0; i < memberCount; ++i)
    {
        auto memberID = SteamMatchmaking()->GetLobbyMemberByIndex(m_sharedData.lobbyID, i);
        ConnectionState state;
        state.steamID = memberID.ConvertToUint64();
        state.actorID = static_cast<Actor::ID>(Actor::ID::PlayerOne + i);

        m_sharedData.clientInformation.updateClient(state);

        //create entry from icon / name and check for lobby owner
        const auto& client = m_sharedData.clientInformation.getClient(i);

        auto rootEntity = m_uiScene.createEntity();
        rootEntity.addComponent<xy::Transform>().setPosition(ItemStartPosition);
        rootEntity.getComponent<xy::Transform>().move(0.f, ItemVerticalSpacing * i);
        rootEntity.addComponent<xy::CommandTarget>().ID;
        rootEntity.addComponent<xy::Callback>().active = true;
        rootEntity.getComponent<xy::Callback>().function = callback;

        auto iconEntity = m_uiScene.createEntity();
        iconEntity.addComponent<xy::Drawable>();
        iconEntity.addComponent<xy::Sprite>(client.avatar);
        iconEntity.addComponent<xy::Transform>().setPosition(0.f, 0.f);
        iconEntity.addComponent<xy::CommandTarget>().ID;
        iconEntity.addComponent<xy::Callback>().active = true;
        iconEntity.getComponent<xy::Callback>().function = callback;
        rootEntity.getComponent<xy::Transform>().addChild(iconEntity.getComponent<xy::Transform>());

        auto nameEntity = m_uiScene.createEntity();
        nameEntity.addComponent<xy::Drawable>();
        nameEntity.addComponent<xy::Text>(font).setString(client.name);
        nameEntity.getComponent<xy::Text>().setCharacterSize(50);
        nameEntity.addComponent<xy::Transform>().setPosition(76.f, 0.f);
        nameEntity.addComponent<xy::CommandTarget>().ID;
        nameEntity.addComponent<xy::Callback>().active = true;
        nameEntity.getComponent<xy::Callback>().function = callback;
        rootEntity.getComponent<xy::Transform>().addChild(nameEntity.getComponent<xy::Transform>());

        if (memberID == owner)
        {
            auto ownerEntity = m_uiScene.createEntity();
            ownerEntity.addComponent<xy::Drawable>();
            ownerEntity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Host];
            ownerEntity.addComponent<xy::Transform>().setPosition(-84.f, 0.f);
            ownerEntity.addComponent<xy::CommandTarget>().ID;
            ownerEntity.addComponent<xy::Callback>().active = true;
            ownerEntity.getComponent<xy::Callback>().function = callback;
            rootEntity.getComponent<xy::Transform>().addChild(ownerEntity.getComponent<xy::Transform>());
        }

        //ready box
        std::string readyState = SteamMatchmaking()->GetLobbyMemberData(m_sharedData.lobbyID, memberID, "ready");

        auto readyEntity = m_uiScene.createEntity();
        readyEntity.addComponent<xy::Transform>().setPosition(1540.f, 0.f);
        readyEntity.addComponent<xy::Drawable>();
        readyEntity.addComponent<xy::Sprite>() = m_sprites[Menu::SpriteID::Checkbox];
        auto bounds = readyEntity.getComponent<xy::Sprite>().getTextureRect();
        if (readyState == "true")
        {
            bounds.top = bounds.height;
            readyEntity.getComponent<xy::Sprite>().setTextureRect(bounds);
        }
        bounds.top = 0.f;
        bounds.left = 0.f;
        readyEntity.addComponent<xy::CommandTarget>();
        readyEntity.addComponent<xy::Callback>().active = true;
        readyEntity.getComponent<xy::Callback>().function = 
            [](xy::Entity entity, float)
        {
            entity.getComponent<xy::CommandTarget>().ID = Menu::CommandID::LobbyItem | Menu::CommandID::ReadyBox;
            entity.getComponent<xy::Callback>().active = false;
        };
        readyEntity.addComponent<CSteamID>() = memberID;
        readyEntity.addComponent<std::uint8_t>() = 0; //in lieu of bool.
        readyEntity.addComponent<xy::UIHitBox>().area = bounds;
        readyEntity.getComponent<xy::UIHitBox>().callbacks[xy::UIHitBox::CallbackID::MouseDown] =
            m_callbackIDs[Menu::CallbackID::ReadyBoxClicked];
        rootEntity.getComponent<xy::Transform>().addChild(readyEntity.getComponent<xy::Transform>());
    }    
}