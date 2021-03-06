/*********************************************************************
(c) Matt Marchant 2019
http://trederia.blogspot.com

osgc - Open Source Game Collection - Zlib license.

This software is provided 'as-is', without any express or
implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.
*********************************************************************/

#include "PluginExport.hpp"
#include "StateIDs.hpp"
#include "MenuState.hpp"
#include "MenuConfirmState.hpp"
#include "GameState.hpp"
#include "PauseState.hpp"
#include "TransitionState.hpp"
#include "ErrorState.hpp"
#include "DialogueState.hpp"
#include "SummaryState.hpp"
#include "IntroState.hpp"
#include "EndingState.hpp"
#include "SharedStateData.hpp"
#include "EditorState.hpp"

#include <xyginext/core/StateStack.hpp>
#include <xyginext/core/Log.hpp>


int begin(xy::StateStack* ss, SharedStateData* sharedData)
{
    *sharedData = std::make_any<SharedData>();
    auto& sd = std::any_cast<SharedData&>(*sharedData);

    ss->registerState<MenuState>(StateID::MainMenu, sd);
    ss->registerState<MenuConfirmState>(StateID::MenuConfirm, sd);
    ss->registerState<GameState>(StateID::Game, sd);
    ss->registerState<PauseState>(StateID::Pause, sd);
    ss->registerState<TransitionState>(StateID::Transition, sd);
    ss->registerState<ErrorState>(StateID::Error, sd);
    ss->registerState<DialogueState>(StateID::Dialogue, sd);
    ss->registerState<SummaryState>(StateID::Summary, sd);
    ss->registerState<IntroState>(StateID::Intro, sd);
    ss->registerState<EndingState>(StateID::Ending, sd);
    ss->registerState<EditorState>(StateID::Editor, sd);

    sd.loadInputBinding();
    sd.loadProgress();

    xy::App::getActiveInstance()->setWindowTitle("Back to Computer!");

#ifdef XY_DEBUG
    //return StateID::Ending;
    //return StateID::Game;
    return StateID::MainMenu;
    //return StateID::Editor;
#else
    return StateID::MainMenu;
#endif //XY_DEBUG
}

void end(xy::StateStack* ss)
{
    ss->unregisterState(StateID::MainMenu);
    ss->unregisterState(StateID::MenuConfirm);
    ss->unregisterState(StateID::Game);
    ss->unregisterState(StateID::Pause);
    ss->unregisterState(StateID::Transition);
    ss->unregisterState(StateID::Error);
    ss->unregisterState(StateID::Dialogue);
    ss->unregisterState(StateID::Summary);
    ss->unregisterState(StateID::Intro);
    ss->unregisterState(StateID::Ending);
    ss->unregisterState(StateID::Editor);
}