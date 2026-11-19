#define MINIAUDIO_IMPLEMENTATION

#include "audio/AudioManager.h"

#include <algorithm>
#include <iostream>


AudioManager::~AudioManager()
{
    shutdown();
}


// Initialize engine and load music.
bool AudioManager::initialize(
    const std::string& filePath,
    bool loop)
{
    shutdown();

    ma_result result = ma_engine_init(nullptr, &engine_);

    if (result != MA_SUCCESS)
    {
        std::cerr << "Audio engine initialization failed: "
                  << ma_result_description(result) << std::endl;

        return false;
    }

    engineInitialized_ = true;

    ma_uint32 flags =
        MA_SOUND_FLAG_STREAM |
        MA_SOUND_FLAG_NO_SPATIALIZATION;

    result = ma_sound_init_from_file(
        &engine_,
        filePath.c_str(),
        flags,
        nullptr,
        nullptr,
        &music_
    );

    if (result != MA_SUCCESS)
    {
        std::cerr << "Music loading failed: "
                  << filePath << std::endl;

        shutdown();
        return false;
    }

    soundInitialized_ = true;

    ma_sound_set_looping(
        &music_,
        loop ? MA_TRUE : MA_FALSE
    );

    ma_sound_set_volume(&music_, 0.7f);

    std::cout << "Music loaded successfully!" << std::endl;

    return true;
}


// Start music.
void AudioManager::play()
{
    if (soundInitialized_)
    {
        ma_sound_start(&music_);
    }
}


// Pause music without resetting its position.
void AudioManager::pause()
{
    if (soundInitialized_)
    {
        ma_sound_stop(&music_);
    }
}


// Resume music from the previous position.
void AudioManager::resume()
{
    if (soundInitialized_)
    {
        ma_sound_start(&music_);
    }
}


// Stop music and return to the beginning.
void AudioManager::stop()
{
    if (soundInitialized_)
    {
        ma_sound_stop(&music_);
        ma_sound_seek_to_pcm_frame(&music_, 0);
    }
}


// Change volume.
void AudioManager::setVolume(float volume)
{
    if (soundInitialized_)
    {
        volume = std::clamp(volume, 0.0f, 1.0f);

        ma_sound_set_volume(&music_, volume);
    }
}


// Check playback status.
bool AudioManager::isPlaying() const
{
    if (!soundInitialized_)
        return false;

    return ma_sound_is_playing(&music_) == MA_TRUE;
}


// Get current playback time in seconds.
float AudioManager::getPlaybackTimeSeconds() const
{
    if (!soundInitialized_)
        return 0.0f;

    ma_uint64 cursor = 0;
    ma_format format;
    ma_uint32 channels = 0;
    ma_uint32 sampleRate = 0;

    ma_result result = ma_sound_get_cursor_in_pcm_frames(
        &music_,
        &cursor
    );

    if (result != MA_SUCCESS)
        return 0.0f;

    result = ma_sound_get_data_format(
        &music_,
        &format,
        &channels,
        &sampleRate,
        nullptr,
        0
    );

    if (result != MA_SUCCESS || sampleRate == 0)
        return 0.0f;

    return static_cast<float>(cursor) /
           static_cast<float>(sampleRate);
}


// Release resources.
void AudioManager::shutdown()
{
    if (soundInitialized_)
    {
        ma_sound_uninit(&music_);
        soundInitialized_ = false;
    }

    if (engineInitialized_)
    {
        ma_engine_uninit(&engine_);
        engineInitialized_ = false;
    }
}