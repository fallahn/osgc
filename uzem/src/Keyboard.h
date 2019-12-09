/*
 * Keyboard.h
 *
 *  Created on: Mar 13, 2015
 *      Author: admin
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <SFML/Window/Keyboard.hpp>

//scan codes
const u32 uzeKbScancodes[][2] = {
	{0x0d,sf::Keyboard::Tab},
	{0x0e,sf::Keyboard::Tilde},// should be `
	{0x12,sf::Keyboard::LShift},
	{0x15,sf::Keyboard::Q},
	{0x16,sf::Keyboard::Num1},
	{0x1a,sf::Keyboard::Z},
	{0x1b,sf::Keyboard::S},
	{0x1c,sf::Keyboard::A},
	{0x1d,sf::Keyboard::W},
	{0x1e,sf::Keyboard::Num2},
	{0x21,sf::Keyboard::C},
	{0x22,sf::Keyboard::X},
	{0x23,sf::Keyboard::D},
	{0x24,sf::Keyboard::E},
	{0x25,sf::Keyboard::Num4},
	{0x26,sf::Keyboard::Num3},
	{0x29,sf::Keyboard::Space},
	{0x2a,sf::Keyboard::V},
	{0x2b,sf::Keyboard::F},
	{0x2c,sf::Keyboard::T},
	{0x2d,sf::Keyboard::R},
	{0x2e,sf::Keyboard::Num5},
	{0x31,sf::Keyboard::N},
	{0x32,sf::Keyboard::B},
	{0x33,sf::Keyboard::H},
	{0x34,sf::Keyboard::G},
	{0x35,sf::Keyboard::Y},
	{0x36,sf::Keyboard::Num6},
	{0x3a,sf::Keyboard::M},
	{0x3b,sf::Keyboard::J},
	{0x3c,sf::Keyboard::U},
	{0x3d,sf::Keyboard::Num7},
	{0x3e,sf::Keyboard::Num8},
	{0x41,sf::Keyboard::Comma},
	{0x42,sf::Keyboard::K},
	{0x43,sf::Keyboard::I},
	{0x44,sf::Keyboard::O},
	{0x45,sf::Keyboard::Num0},
	{0x46,sf::Keyboard::Num9},
	{0x49,sf::Keyboard::Period},
	{0x4a,sf::Keyboard::Slash},
	{0x4b,sf::Keyboard::L},
	{0x4c,sf::Keyboard::Semicolon},
	{0x4d,sf::Keyboard::P},
	{0x4e,sf::Keyboard::Hyphen},
	{0x52,sf::Keyboard::Quote},
	{0x54,sf::Keyboard::LBracket},
	{0x55,sf::Keyboard::Equal},
	{0x59,sf::Keyboard::RShift},
	{0x5a,sf::Keyboard::Return},
	{0x5b,sf::Keyboard::RBracket},
	{0x5d,sf::Keyboard::BackSlash},
	{0x66,sf::Keyboard::BackSpace},
	{0x69,sf::Keyboard::Numpad1},
	{0x6b,sf::Keyboard::Numpad4},
	{0x6c,sf::Keyboard::Numpad7},
	{0x70,sf::Keyboard::Numpad0},
	{0x71,sf::Keyboard::Period},
	{0x72,sf::Keyboard::Numpad2},
	{0x73,sf::Keyboard::Numpad5},
	{0x74,sf::Keyboard::Numpad6},
	{0x75,sf::Keyboard::Numpad8},
	{0x79,sf::Keyboard::Add},
	{0x7a,sf::Keyboard::Numpad3},
	{0x7b,sf::Keyboard::Subtract},
	{0x7c,sf::Keyboard::Multiply},
	{0x7d,sf::Keyboard::Numpad9},
	{0,0}
};





#endif /* KEYBOARD_H_ */
