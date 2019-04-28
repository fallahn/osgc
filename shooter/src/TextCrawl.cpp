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

#include "TextCrawl.hpp"

#include <xyginext/core/App.hpp>

#include <SFML/Graphics/Font.hpp>

namespace
{
    const std::string textString = 
 R"(A distant earth colony has come
under attack from alien insects
for some reason! Their only hope
is a maverick drone pilot, armed
with drop-bombs and some second
hand batteries. Beat back the
alien infestation from above and
save as many humans as you can!)";
}

TextCrawl::TextCrawl(sf::Font& font)
{
    m_text.setFont(font);
    m_text.setFillColor(sf::Color::Yellow);
    m_text.setString(textString);
    m_text.setPosition(2.f, 540.f);

    m_texture.create(960, 540);
}

//public
bool TextCrawl::update(float dt)
{
    m_text.move(0.f, -20.f * dt);

    m_texture.clear(sf::Color::Transparent);
    m_texture.draw(m_text);
    m_texture.display();

    return m_text.getPosition().y < -220.f;
}