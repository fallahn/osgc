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

#include "TimeTrialState.hpp"

TimeTrialState::TimeTrialState(xy::StateStack& ss, xy::State::Context ctx, SharedData& sd)
    : xy::State(ss, ctx),
    m_sharedData(sd),
    m_scene(ctx.appInstance.getMessageBus())
{
    launchLoadingScreen();
    initScene();
    loadResources();


    quitLoadingScreen();
}

//public
bool TimeTrialState::handleEvent(const sf::Event& evt)
{
    m_scene.forwardEvent(evt);

    return true;
}

void TimeTrialState::handleMessage(const xy::Message& msg)
{
    m_scene.forwardMessage(msg);
}

bool TimeTrialState::update(float dt)
{
    m_scene.update(dt);
    return true;
}

void TimeTrialState::draw()
{

}

//private
void TimeTrialState::initScene()
{
    auto& mb = getContext().appInstance.getMessageBus();
}

void TimeTrialState::loadResources()
{

}