/*
(The MIT License)

Copyright (c) 2019 by
Matt Marchant

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "AvrAudio.hpp"

#include <SFML/System/Sleep.hpp>

AvrAudio::AvrAudio()
    : m_outputBuffer()
{
    initialize(1, 15734);
}

bool AvrAudio::onGetData(Chunk& chunk)
{
    m_mutex.lock();
    auto size = std::min(m_outputBuffer.size(), m_ringBuffer.size());
    for (auto i = 0u; i < size; ++i)
    {
        m_outputBuffer[i] = m_ringBuffer.pop_front();
    }
    m_mutex.unlock();

    //pack with 0 if we have no data
    if (size == 0)
    {
        for (auto i = 0u; i < m_outputBuffer.size(); ++i)
        {
            m_outputBuffer[i] = 0;
        }
        size = m_outputBuffer.size();
    }

    chunk.sampleCount = size;
    chunk.samples = m_outputBuffer.data();

    return true;
}

void AvrAudio::pushData(std::uint8_t data)
{
    std::int16_t value = data;
    value -= 0x80;
    value <<= 8;

    while (m_ringBuffer.full())
    {
        sf::sleep(sf::milliseconds(1));
    }

    sf::Lock lock(m_mutex);
    m_ringBuffer.push_back(value);
}
