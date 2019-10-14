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

#include <xyginext/core/Message.hpp>

#include <SFML/System/Vector2.hpp>
#include <SFML/Graphics/Rect.hpp>

namespace Menu
{
    namespace CommandID
    {
        enum
        {
            MainNode = 0x1,
            LobbyNode = 0x2,
            BrowserNode = 0x4,
            LobbyItem = 0x8,
            SeedText = 0x10,
            BrowserItem = 0x20,
            StartButton = 0x40,
            OptionsNode = 0x80,

            HostNameNode = 0x100,
            JoinNameNode = 0x200,

            NameText = 0x400,
            IPText = 0x800,
            HostIcon = 0x1000,

            ChatInText = 0x2000,
            ChatOutText = 0x4000,
            OptionsInfoText = 0x8000,
            KeybindButton = 0x10000,
            JoybindButton = 0x20000
        };
    }

    namespace MessageID
    {
        enum
        {
            UIMessage = xy::Message::Count
        };
    }

    struct UIEvent final
    {
        enum
        {
            Checked,
            Unchecked
        }type;
        sf::Uint32 id = 0;
    };

    //namespace ControlID
    //{
    //    enum
    //    {

    //    };
    //}

    namespace SpriteDepth
    {
        enum
        {
            Far = 0,
            Mid,
            Near
        };
    }

    namespace SpriteID
    {
        enum
        {
            Host,
            Checkbox,
            PlayerOne,
            PlayerTwo,
            PlayerThree,
            PlayerFour,
            Bot,
            TitleBar,
            PlayerFrame,
            RandomButton,
            TextInput,
            ChatBox,
            ChatButton,
            TreasureMap,
            ButtonBackground,
            ArrowLeft,
            ArrowRight,
            Count
        };
    }

    namespace CallbackID
    {
        enum
        {
            TextSelected,
            TextUnselected,
            CheckboxClicked,
            BrowserItemSelected,
            BrowserItemDeselected,
            BrowserItemClicked,
            ReadyBoxClicked,
            Count
        };
    }

    namespace ChatMessageID
    {
        enum
        {
            StartGame,
            ChatText
        };
    }

    static const sf::Vector2f OffscreenPosition(0.f, -1080.f);
    static const sf::FloatRect ButtonArea({ 0.f, 0.f, 300.f, 70.f });
    static const sf::Vector2f StartButtonPosition(1364.f, 940.f);
    static const sf::Vector2f StartButtonPositionHidden(2890.f, 960.f);
    static const sf::Vector2f BackButtonPosition(20.f, 940.f);
    static const sf::Vector2f SeedPosition(210.f, 738.f);
    static const sf::Vector2f ChatInputPosition(210.f, 830.f);
    static const sf::Vector2f ChatBoxPosition(940.f, 738.f);
    static const sf::Vector2f FriendsPosition(120.f, 830.f);
    static const sf::Vector2f TitlePosition(xy::DefaultSceneSize.x / 2.f, 168.f);

    static const float SliderSpeed = 8.f;

    const std::size_t MaxChatChar = 40;
    const std::size_t MaxSeedChar = 15;
}