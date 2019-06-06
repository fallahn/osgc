///resource managers///
#ifndef RESOURCES_H_
#define RESOURCES_H_

#include <SFML/System/NonCopyable.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Graphics/Font.hpp>

#include <memory>
#include <array>
#include <iostream>

namespace Game
{
	template <class T>
	class BaseResource : private sf::NonCopyable
	{
	public:
		BaseResource()
		{
		}

		virtual ~BaseResource(){};

		T& Get(const std::string& path = "default")
		{
			//if we have a valid path check current resources and return if found
			if(!path.empty())
			{
				auto r = m_resources.find(path);
				if(r != m_resources.end())
				{
					//Console::lout << "Found resource: " << path << std::endl;
					return *r->second;
				}
			}

			//else attempt to load from file
			std::unique_ptr<T> r = std::unique_ptr<T>(new T());
			if(path.empty() || !r->loadFromFile(path))
			{
				std::cerr << "Unable to load resource: " << path/* << std::endl*/;
				m_resources[path] = m_errorHandle(); //error handle should return message endl
			}
			else
			{
				//Console::lout << "Loaded Resource: " << path << std::endl;
				m_resources[path] = std::move(r);
			}

			//TODO - we could check for map size here and flush if over certain size

			return *m_resources[path];
		}


	protected:
		virtual std::unique_ptr<T> m_errorHandle() = 0;

	private:
		std::map<std::string, std::unique_ptr<T> > m_resources;
	};


	class TextureResource final : public BaseResource<sf::Texture>
	{
	private:
		std::unique_ptr<sf::Texture> m_errorHandle()
		{
			std::cerr << " returning default texture..." << std::endl;

			std::unique_ptr<sf::Texture> t(new sf::Texture());
			sf::Image i;
			i.create(20u, 20u, sf::Color::Magenta);
			t->loadFromImage(i);
			return std::move(t);
		}
	};

	class SoundBufferResource final : public BaseResource<sf::SoundBuffer>
	{
	public:
		SoundBufferResource()
		{
			//create a single cycle square wave @ 441Hz
			int i = 0;
			for(i; i < 50; i++)
				m_byteArray[i] = 16383;

			for(i; i < 100; i++)
				m_byteArray[i] = -16384;
		}
	private:
		std::array<sf::Int16, 100> m_byteArray;
		std::unique_ptr<sf::SoundBuffer> m_errorHandle()
		{
			std::cerr << " returning default sound buffer..." << std::endl;

			//load the byte array into a new buffer
			std::unique_ptr<sf::SoundBuffer> b(new sf::SoundBuffer);
			b->loadFromSamples(&m_byteArray[0], m_byteArray.size(), 1u, 44100);
			return std::move(b);
		}
	};

	class FontResource final : public BaseResource<sf::Font>
	{
	public:
		FontResource();
	private:
		sf::Font m_font;
		std::unique_ptr<sf::Font> m_errorHandle();
	};

	//TODO image and shader class
};


#endif