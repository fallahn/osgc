///source for audio manager///
#include <Game/AudioManager.h>
#include <SFML/Audio/Listener.hpp>

using namespace Game;

namespace
{
	const float ListenerZ = 300.f;
	const float MinDistance2D = 300.f;
	const float MinDistance3D = std::sqrt(MinDistance2D * MinDistance2D + ListenerZ * ListenerZ);
}


//ctor
AudioManager::AudioManager()
	: m_globalEffectsVolume (100.f),
	m_globalMusicVolume		(100.f),
	m_attenuation			(4.f)
{
	//map enums to files
	m_buffers[Audio::Eliminated] = m_soundBufferResource.Get("assets/sound/fx/eliminated.wav");
	m_buffers[Audio::Explode] = m_soundBufferResource.Get("assets/sound/fx/vehicles/die.wav");
	m_buffers[Audio::Impact] = m_soundBufferResource.Get("assets/sound/fx/vehicles/collide.wav");
	m_buffers[Audio::LapLine] = m_soundBufferResource.Get("assets/sound/fx/vehicles/lap.wav");
	m_buffers[Audio::RaceWin] = m_soundBufferResource.Get("assets/sound/fx/round_end.wav");
	m_buffers[Audio::Start] = m_soundBufferResource.Get("assets/sound/fx/start.wav");
	m_buffers[Audio::Success] = m_soundBufferResource.Get("assets/sound/fx/success.wav");
	m_buffers[Audio::SuddenDeath] = m_soundBufferResource.Get("assets/sound/fx/sudden_death.wav");
	m_buffers[Audio::Skid] = m_soundBufferResource.Get("assets/sound/fx/vehicles/skid.wav");
	m_buffers[Audio::Spark01] = m_soundBufferResource.Get("assets/sound/fx/sparks/spark01.wav");
	m_buffers[Audio::Spark02] = m_soundBufferResource.Get("assets/sound/fx/sparks/spark02.wav");
	m_buffers[Audio::Spark03] = m_soundBufferResource.Get("assets/sound/fx/sparks/spark03.wav");
	m_buffers[Audio::Spark04] = m_soundBufferResource.Get("assets/sound/fx/sparks/spark04.wav");

	m_musicPaths[Audio::Title] = "assets/sound/music/menu.ogg";
	m_musicPaths[Audio::Map01] = "assets/sound/music/map01.ogg";
	m_musicPaths[Audio::Map02] = "assets/sound/music/map02.ogg";
	m_musicPaths[Audio::Map03] = "assets/sound/music/map03.ogg";
	m_musicPaths[Audio::Map04] = "assets/sound/music/map04.ogg";
	m_musicPaths[Audio::Map05] = "assets/sound/music/map05.ogg";
}


//public
void AudioManager::PlayEffect(Audio::Effect effect)
{
	PlayEffect(effect, GetListenerPosition());
}

void AudioManager::PlayEffect(Audio::Effect effect, const sf::Vector2f& position)
{
	
	m_sounds.push_back(sf::Sound());
	sf::Sound& sound = m_sounds.back();

	sound.setBuffer(m_buffers[effect]);
	sound.setPosition(position.x, -position.y, 0.f); //OpenAL y coords are inverse to sfml
	sound.setAttenuation(m_attenuation);
	sound.setMinDistance(MinDistance3D);
	sound.setVolume(m_globalEffectsVolume);
	sound.play();
}

void AudioManager::RemoveStoppedSounds()
{
	m_sounds.remove_if([](const sf::Sound& s)
	{
		return s.getStatus() == sf::Sound::Stopped;
	});
}

void AudioManager::SetListenerPosition(const sf::Vector2f& position)
{
	sf::Listener::setPosition(position.x, -position.y, ListenerZ);
}

sf::Vector2f AudioManager::GetListenerPosition()
{
	sf::Vector3f position = sf::Listener::getPosition();
	return sf::Vector2f(position.x, -position.y);
}

void AudioManager::SetGlobalEffectsVolume(float volume)
{
	m_globalEffectsVolume = volume;
	for(auto& s : m_sounds)
		s.setVolume(m_globalEffectsVolume);
}

float AudioManager::GetGlobalEffectsVolume() const
{
	return m_globalEffectsVolume;
}

sf::SoundBuffer& AudioManager::GetEffectBuffer(std::string path)
{
	return m_soundBufferResource.Get(path);
}

void AudioManager::SetAttenuation(float a)
{
	m_attenuation = a;
}

void AudioManager::PlayMusic(Audio::Music music)
{
	if(m_music.openFromFile(m_musicPaths[music]))
	{
		m_music.setVolume(m_globalMusicVolume);
		m_music.setLoop(true);
		m_music.play();
	}
}

void AudioManager::PauseMusic(bool paused)
{
	if(paused) m_music.pause();
	else m_music.play();
}

void AudioManager::StopMusic()
{
	m_music.stop();
}

void AudioManager::SetGlobalMusicVolume(float volume)
{
	m_globalMusicVolume = volume;
	m_music.setVolume(m_globalMusicVolume);
}

float AudioManager::GetGlobalMusicVolume() const
{
	return m_globalMusicVolume;
}