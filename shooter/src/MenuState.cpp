/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - License TBD


*********************************************************************/

#include "MenuState.hpp"
#include "StateIDs.hpp"

#include <xyginext/core/Log.hpp>

MenuState::MenuState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State(ss, ctx)
{
    xy::Logger::log("Menu state!", xy::Logger::Type::Info);
}

//public
bool MenuState::handleEvent(const sf::Event&)
{
    return true;
}

void MenuState::handleMessage(const xy::Message&)
{

}

bool MenuState::update(float)
{
    return true;
}

void MenuState::draw()
{

}

xy::StateID MenuState::stateID() const
{
    return StateID::MainMenu;
}