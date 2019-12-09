/*********************************************************************
(c) Matt Marchant 2019

xygineXT - Zlib license.

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

#include "MainState.hpp"
#include "States.hpp"

#include "uzerom.h"
#include "logo.h"

#include <xyginext/core/App.hpp>
#include <xyginext/gui/Gui.hpp>
#include <xyginext/util/String.hpp>

#include <SFML/Audio/Listener.hpp>

#include <string>

namespace
{
    const int cycles = 100000000;
}

MainState::MainState(xy::StateStack& ss, xy::State::Context ctx)
    : xy::State     (ss, ctx),
    m_showOptions   (true),
    m_hideHelpText  (false),
    m_thread        (&MainState::emulate, this),
    m_runEmulation  (false)
{
    launchLoadingScreen();

    xy::AudioMixer::setLabel("Sound", 0);

    m_uzebox.init_gui();
    m_uzebox.randomSeed = time(NULL);
    std::srand(m_uzebox.randomSeed);//used for the watchdog timer entropy

    loadResources();

    //imgui window for opts / loading roms
    //display ROM data and other console output
    m_romInfo = "No ROM loaded.";
    registerWindow([&]() 
        {
            if (m_showOptions)
            {
                static bool LoadRom = false;
                static bool CloseRom = false;
                static bool SoftReset = false;

                ImGui::SetNextWindowSize({ 280.f, 400.f }/*, ImGuiCond_FirstUseEver*/);
                if (ImGui::Begin("Uzem", &m_showOptions, ImGuiWindowFlags_MenuBar))
                {
                    if (ImGui::BeginMenuBar())
                    {
                        if (ImGui::BeginMenu("File"))
                        {
                            ImGui::MenuItem("Load ROM", nullptr, &LoadRom);
                            ImGui::MenuItem("Close ROM", nullptr, &CloseRom);
                            ImGui::MenuItem("Choose EEPROM", nullptr, nullptr);
                            ImGui::EndMenu();
                        }

                        if (ImGui::BeginMenu("Emulation"))
                        {
                            //ImGui::MenuItem("Soft Reset", nullptr, &SoftReset);
                            ImGui::MenuItem("Quit", nullptr, nullptr);
                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }


                    ImGui::Text("%s", "Current EEPROM: None");
                    ImGui::Checkbox("Hide Hint", &m_hideHelpText);

                    ImGui::Separator();
                    ImGui::Text("%s", m_romInfo.c_str());
                    ImGui::Text("%s", "Key binds go here");
                    ImGui::End();
                }

                if (LoadRom)
                {
                    openRom();
                    LoadRom = false;
                }

                if (CloseRom)
                {
                    closeRom();
                    CloseRom = false;
                }

                if (SoftReset)
                {
                    m_uzebox.softReset();
                    SoftReset = false;
                }
            }
        });

    updateView();

    quitLoadingScreen();
}

MainState::~MainState()
{
    if (m_runEmulation)
    {
        m_runEmulation = false;
        m_thread.wait();
    }
}

bool MainState::handleEvent(const sf::Event& evt)
{
    if (xy::ui::wantsKeyboard()
        || xy::ui::wantsMouse())
    {
        return true;
    }

    if (evt.type == sf::Event::KeyReleased)
    {
        switch (evt.key.code)
        {
        default: break;
        case sf::Keyboard::F2:
            m_showOptions = !m_showOptions;
            break;
        }
    }


    //else buffer the events into the emulator for parsing
    m_uzebox.bufferEvent(evt);

    return true;
}

void MainState::handleMessage(const xy::Message& msg)
{
    if (msg.id == xy::Message::AudioMessage)
    {
        const auto& data = msg.getData<xy::Message::AudioEvent>();
        if (data.type == xy::Message::AudioEvent::ChannelVolumeChanged)
        {
            m_uzebox.setVolume(xy::AudioMixer::getVolume(0) * 100.f);
        }
    }
    else if (msg.id == xy::Message::WindowMessage)
    {
        const auto& data = msg.getData<xy::Message::WindowEvent>();
        if (data.type == xy::Message::WindowEvent::Resized)
        {
            updateView();
        }
    }
}

bool MainState::update(float dt)
{
    sf::Listener::setGlobalVolume(xy::AudioMixer::getMasterVolume() * 100.f);
    return true;
}

void MainState::draw()
{
    auto rw = getContext().appInstance.getRenderWindow();
    rw->setView(m_view);
    rw->draw(m_uzebox.m_sprite);

    //set to default view to maintain help text scale.
    if (!m_hideHelpText)
    {
        rw->setView(rw->getDefaultView());
        rw->draw(m_helpText);
    }
}

xy::StateID MainState::stateID() const
{
    return States::Main;
}

//private
void MainState::emulate()
{
    while (m_runEmulation)
    {
        auto remain = cycles;
        while (remain > 0 && m_runEmulation)
        {
            remain -= m_uzebox.exec();
        }
    }
    m_uzebox.softReset();
}

void MainState::openRom()
{
    closeRom();

    auto romPath = xy::FileSystem::openFileDialogue("", "uze,hex");

    if (m_uzebox.eepromFile)
    {
        m_uzebox.LoadEEPROMFile(m_uzebox.eepromFile);
    }

    unsigned char* buffer = reinterpret_cast<unsigned char*>(m_uzebox.progmem);

    auto fileExtension = xy::Util::String::toLower(xy::FileSystem::getFileExtension(romPath));

    if (fileExtension == ".uze")
    {
        if (isUzeromFile(romPath.data()))
        {
            RomHeader uzeRomHeader;
            if (!loadUzeImage(romPath.data(), &uzeRomHeader, buffer))
            {
                xy::Logger::log("Cannot open UzeRom file " + romPath, xy::Logger::Type::Error);
                return;
            }
            // enable mouse support if required
            if (uzeRomHeader.mouse) 
            {
                m_uzebox.pad_mode = avr8::SNES_MOUSE;
                xy::Logger::log("Mouse support enabled", xy::Logger::Type::Info);
            }

            //update the ROM info
            std::stringstream ss;
            ss << "Loaded:\n";
            ss << uzeRomHeader.name << "\n";
            ss << uzeRomHeader.author << "\n";
            ss << uzeRomHeader.year << "\n";
            ss << uzeRomHeader.description << "\n";

            if (uzeRomHeader.target == 0)
            {
                ss << "Uzebox 1.0 - ATmega644\n";
            }
            else
            {
                //although loading should have failed and quit before we ever got here...
                ss << "Error: Uzebox 2.0 ROM images are not supported.\n";
            }

            m_romInfo = ss.str();
        }
        else 
        {
            xy::Logger::log("Could not load ROM file - bad header?", xy::Logger::Type::Error);
            return;
        }

    }
    else if(fileExtension == ".hex")
    {
        xy::Logger::log("Loading hex file...", xy::Logger::Type::Info);
        if (!loadHex(romPath.c_str(), buffer))
        {
            xy::Logger::log("Could not open hex file " + romPath, xy::Logger::Type::Error);
            return;
        }
        m_romInfo = "Loaded hex...\nNo info available.";
    }
    else
    {
        xy::Logger::log(fileExtension + " - not a supported file type.", xy::Logger::Type::Error);
        m_romInfo = "No ROM loaded.";
        return;
    }

    m_uzebox.decodeFlash();

    //get rom name without extension
    m_uzebox.romName = xy::FileSystem::getFileName(romPath);

    if (m_uzebox.SDpath.empty())
    {
        m_uzebox.SDpath = xy::FileSystem::getFilePath(romPath);
    }

    m_runEmulation = true;
    m_thread.launch();
}

void MainState::closeRom()
{
    if (m_runEmulation)
    {
        m_runEmulation = false;
        m_thread.wait();
    }

    //TODO set shader to render noise on output texture
}

void MainState::loadResources()
{
    auto fontID = m_resources.load<sf::Font>("buns");
    auto& font = m_resources.get<sf::Font>(fontID);

    m_helpText.setFont(font);
    m_helpText.setPosition(10.f, 10.f);
    m_helpText.setString("F1: Configuration  F2: Options");
    m_helpText.setCharacterSize(16);
    m_helpText.setFillColor(sf::Color(3,65,84));
}

void MainState::updateView()
{
    auto ctx = getContext();
    auto winSize = sf::Vector2f(ctx.renderWindow.getSize());

    float windowRatio = winSize.x / winSize.y;
    float viewRatio = 4.f / 3.f;
    float sizeX = 1.f;
    float sizeY = 1;
    float posX = 0.f;
    float posY = 0.f;

    if (windowRatio > viewRatio)
    {
        sizeX = viewRatio / windowRatio;
        posX = (1.f - sizeX) / 2.f;
    }
    else
    {
        sizeY = windowRatio / viewRatio;
        posY = (1.f - sizeY) / 2.f;
    }

    sf::View view;
    view.setSize(1440.f, 1080.f);
    view.setCenter(720.f, 540.f);
    view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));

    ctx.renderWindow.setIcon(32, 32, reinterpret_cast<const sf::Uint8*>(&logo));

    m_view = view;
}