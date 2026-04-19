#pragma once

// Standard library
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// Windows / XAudio2
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xaudio2.h>

// -----------------------------------------------------------------------
// Sound IDs — add new sounds here as needed
// -----------------------------------------------------------------------
enum class SoundID : int {
    Jump   = 0,
    Stomp  = 1,
    Hurt   = 2,
    Die    = 3,
    Coin   = 4,
    Count
};

class SoundManager {
public:
    SoundManager();
    ~SoundManager();

    // Non-copyable
    SoundManager(const SoundManager&)            = delete;
    SoundManager& operator=(const SoundManager&) = delete;

    // Returns false if XAudio2 failed to init (e.g. no audio device).
    // Game should continue running silently in that case.
    bool Init();

    // Load a .wav file and register it under the given SoundID.
    // Supports PCM integer and IEEE float formats.
    // Returns false on failure (file not found, unsupported format).
    bool Load(SoundID id, const std::string& path);

    // Fire-and-forget playback. Creates a temporary source voice.
    // volume = 1.0 is 100%. Silently ignored if sound was not loaded.
    void Play(SoundID id, float volume = 1.0f);

    // Master volume [0.0, 1.0]
    void SetMasterVolume(float volume);

    bool IsReady() const { return _engine != nullptr; }

private:
    struct WavBuffer {
        std::vector<uint8_t> pcm_data;
        WAVEFORMATEX         format = {};
        bool                 loaded = false;
    };

    // Parse a canonical PCM/IEEE RIFF .wav file into WavBuffer.
    // Returns false on parse error.
    static bool ParseWav(const std::string& path, WavBuffer& out);

    IXAudio2*              _engine          = nullptr;
    IXAudio2MasteringVoice* _mastering_voice = nullptr;

    static constexpr int SOUND_COUNT = static_cast<int>(SoundID::Count);
    WavBuffer _sounds[SOUND_COUNT];
};
