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

#pragma once

#include "OBJ_Loader.h"
#include "MatrixPool.hpp"

#include <xyginext/core/State.hpp>
#include <xyginext/ecs/Scene.hpp>
#include <xyginext/gui/GuiClient.hpp>
#include <xyginext/resources/ResourceHandler.hpp>
#include <xyginext/resources/ShaderResource.hpp>

#include <string>

class ModelState final : public xy::State, public xy::GuiClient
{
public:
    ModelState(xy::StateStack&, xy::State::Context);

    bool handleEvent(const sf::Event&) override;

    void handleMessage(const xy::Message&) override;

    bool update(float) override;

    void draw() override;

    xy::StateID stateID() const override;

private:
    xy::Scene m_uiScene;

    objl::Loader m_objLoader;
    bool m_fileLoaded;
    std::string m_fileData;

    xy::ResourceHandler m_resources;
    xy::ShaderResource m_shaders;
    MatrixPool m_matrixPool;

    void initScene();
    void parseVerts();
    void exportModel(const std::string&);

    void setCamera(std::int32_t);
};