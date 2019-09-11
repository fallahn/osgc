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

#include <SFML/Graphics/Rect.hpp>
#include <SFML/System/Vector2.hpp>

template <typename T>
sf::Vector2<T> operator * (sf::Vector2<T> l, sf::Vector2<T> r)
{
    return { l.x * r.x, l.y * r.y };
}

template <typename T>
sf::Rect<T> operator * (sf::Rect<T> l, T r)
{
    return { l.left * r, l.top * r, l.width * r, l.height * r };
}