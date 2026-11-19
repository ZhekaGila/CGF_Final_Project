#pragma once

#include <string>
#include "miniaudio.h"

class AudioManager
{
public:

    AudioManager() = default;

    ~AudioManager();

    // Disable copying of audio engine resources.
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Initialize audio engine and load a music file.
    bool initialize(const std::string& filePath, bool loop = true);

    // Music controls.
    void play();
    void pause();
    void resume();
    void stop();

    // Volume control (0.0 - 1.0).
    void setVolume(float volume);

    // Playback information.
    bool isPlaying() const;
    float getPlaybackTimeSeconds() const;

    // Release audio resources.
    void shutdown();

private:

    ma_engine engine_{};
    ma_sound music_{};

    bool engineInitialized_ = false;
    bool soundInitialized_ = false;
};