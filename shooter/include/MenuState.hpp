/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#pragma once

#include <xyginext/core/State.hpp>

class MenuState final : public xy::State
{
public:
    MenuState(xy::StateStack&, xy::State::Context);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:

};