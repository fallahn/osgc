///creates an audio manager for playing effects an music///
#ifndef AUDIO_MAN_H_
#define AUDIO_MAN_H_

#include <SFML/Audio/SoundBuffer.hpp>
#include <SFML/Audio/Sound.hpp>
#include <SFML/Audio/Music.hpp>

#include <Game/ResourceManager.h>
#include <list>

namespace Game
{
	namespace Audio
	{
		enum Effect
		{
			// name of effects go here
			Eliminated,
			Explode,
			Impact,
			LapLine,
			RaceWin,
			Start,
			Success,
			SuddenDeath,
			Skid,
			Spark01,
			Spark02,
			Spark03,
			Spark04
		};

		enum Music
		{
			//name of music go here
			Title,
			Map01,
			Map02,
			Map03,
			Map04,
			Map05
		};
	}
	
	class AudioManager : private sf::NonCopyable
	{
	public:
		AudioManager();

		//----sfx----//

		void PlayEffect(Audio::Effect effect);
		void PlayEffect(Audio::Effect effect, const sf::Vector2f& position);
		void RemoveStoppedSounds();
		void SetListenerPosition(const sf::Vector2f& position);
		sf::Vector2f GetListenerPosition();
		void SetGlobalEffectsVolume(float volume);
		float GetGlobalEffectsVolume() const;
		void SetAttenuation(float a);

		//if we want to externally manage sounds like engines use the same resource
		sf::SoundBuffer& GetEffectBuffer(std::string path);

		//---music---//

		void PlayMusic(Audio::Music music);
		void PauseMusic(bool paused);
		void StopMusic();
		void SetGlobalMusicVolume(float volume);
		float GetGlobalMusicVolume() const;

	private:
		//----sfx----//

		SoundBufferResource m_soundBufferResource;
		std::list<sf::Sound> m_sounds;
		std::map<Audio::Effect, sf::SoundBuffer> m_buffers;
		float m_globalEffectsVolume;
		float m_attenuation;

		//---music---//

		sf::Music m_music; //currently playing music
		std::map<Audio::Music, std::string> m_musicPaths;
		float m_globalMusicVolume;
	};
}


#endif