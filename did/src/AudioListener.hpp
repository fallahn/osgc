/*-----------------------------------------------------------------------

Matt Marchant 2017
http://trederia.blogspot.com

crogine - Zlib license.

This software is provided 'as-is', without any express or
implied warranty.In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute
it freely, subject to the following restrictions :

1. The origin of this software must not be misrepresented;
you must not claim that you wrote the original software.
If you use this software in a product, an acknowledgment
in the product documentation would be appreciated but
is not required.

2. Altered source versions must be plainly marked as such,
and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any
source distribution.

-----------------------------------------------------------------------*/

#ifndef CRO_AUDIO_LISTENER_HPP_
#define CRO_AUDIO_LISTENER_HPP_

#include <algorithm>

//namespace cro
//{
    /*!
    \brief Audio Listener component.
    The AudioListener component defines the point at which the
    active AudioSystem hears any playing AudioSources. Only one listener
    is active at a time, by default on the Scene's active camera entity.
    When using the OpenAL backend spatialisation is calculated based
    on an AudioSource's position relative to the active AudioListener.
    */
    class AudioListener final
    {
    public:
        /*!
        \brief Sets the overall volume of the Listener.
        This will affect the perceived volume of all sounds
        being heard by the active listener.
        A volume of 0 will mute all audio, 1 plays back at 'normal' volume
        and anything greater will amplify any sounds heard by the listener.
        */
        void setVolume(float volume)
        {
            m_volume = std::max(0.f, volume);
        }

        /*!
        \brief Returns the current volume of the listener
        */
        float getVolume() const
        {
            return m_volume;
        }

    private:
        friend class AudioSystem;
        float m_volume = 1.f;
    };
//}

#endif //CRO_AUDIO_LISTENER_HPP_