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

#include "StateIDs.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/gui/GuiClient.hpp>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>

struct SharedStateData;
class ErrorState final : public xy::State, public xy::GuiClient
{
public:
    ErrorState(xy::StateStack&, xy::State::Context, const SharedStateData&);

    xy::StateID stateID() const override { return StateID::Error; }
    bool handleEvent(const sf::Event&) override;
    void handleMessage(const xy::Message&) override;
    bool update(float) override;
    void draw() override;

private:
    sf::Texture m_backgroundTexture;
    sf::Sprite m_backgroundSprite;
    std::string m_message;
};