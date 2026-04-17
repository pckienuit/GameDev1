#include "SoundManager.h"

#include <fstream>
#include <cstring>

// -----------------------------------------------------------------------
// RIFF chunk parsing helpers
// -----------------------------------------------------------------------
namespace {

// Read 4-byte FourCC tag from stream
bool ReadTag(std::ifstream& f, char tag[4]) {
    return static_cast<bool>(f.read(tag, 4));
}

// Read little-endian uint32 from stream
bool ReadU32(std::ifstream& f, uint32_t& out) {
    return static_cast<bool>(f.read(reinterpret_cast<char*>(&out), 4));
}

} // namespace

// -----------------------------------------------------------------------
// SoundManager — Init / Shutdown
// -----------------------------------------------------------------------
SoundManager::SoundManager() = default;

SoundManager::~SoundManager() {
    if (_mastering_voice) {
        _mastering_voice->DestroyVoice();
        _mastering_voice = nullptr;
    }
    if (_engine) {
        _engine->Release();
        _engine = nullptr;
    }
    CoUninitialize();
}

bool SoundManager::Init() {
    // XAudio2 requires COM — safe to call multiple times
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        OutputDebugStringA("[SoundManager] CoInitializeEx failed\n");
        return false;
    }

    hr = XAudio2Create(&_engine, 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr)) {
        OutputDebugStringA("[SoundManager] XAudio2Create failed — running silently\n");
        return false;
    }

    hr = _engine->CreateMasteringVoice(&_mastering_voice);
    if (FAILED(hr)) {
        OutputDebugStringA("[SoundManager] CreateMasteringVoice failed — running silently\n");
        _engine->Release();
        _engine = nullptr;
        return false;
    }

    return true;
}

void SoundManager::SetMasterVolume(float volume) {
    if (_mastering_voice)
        _mastering_voice->SetVolume(volume);
}

// -----------------------------------------------------------------------
// WAV parsing — handles RIFF PCM and IEEE float
// -----------------------------------------------------------------------
bool SoundManager::ParseWav(const std::string& path, WavBuffer& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;

    char tag[4];
    uint32_t chunk_size = 0;

    // RIFF header
    if (!ReadTag(f, tag) || std::strncmp(tag, "RIFF", 4) != 0) return false;
    ReadU32(f, chunk_size); // total size — ignored here
    if (!ReadTag(f, tag) || std::strncmp(tag, "WAVE", 4) != 0) return false;

    bool found_fmt  = false;
    bool found_data = false;

    // Walk chunks until both fmt and data are found
    while (!found_data && f) {
        char   sub_tag[4] = {};
        uint32_t sub_size = 0;
        if (!ReadTag(f, sub_tag)) break;
        if (!ReadU32(f, sub_size)) break;

        // Align chunk size to 2 bytes (RIFF spec)
        const uint32_t aligned = sub_size + (sub_size & 1u);

        if (std::strncmp(sub_tag, "fmt ", 4) == 0) {
            // Read the raw fmt bytes into a temp buffer
            std::vector<uint8_t> fmt_raw(sub_size);
            f.read(reinterpret_cast<char*>(fmt_raw.data()), sub_size);
            if (aligned > sub_size)
                f.seekg(aligned - sub_size, std::ios::cur);

            // Copy into WAVEFORMATEX (safe: copy only min of both sizes)
            const size_t copy_bytes = min(sub_size, static_cast<uint32_t>(sizeof(WAVEFORMATEX)));
            std::memcpy(&out.format, fmt_raw.data(), copy_bytes);
            out.format.cbSize = 0;

            // WAVE_FORMAT_EXTENSIBLE (0xFFFE): treat as PCM — XAudio2 needs WAVE_FORMAT_PCM (1)
            // The sub-format GUID lives at byte 24, but for game audio PCM/float is safe to assume.
            if (out.format.wFormatTag == 0xFFFE && sub_size >= 28) {
                // Read the actual format tag from SubFormat GUID (bytes 24-25)
                const uint16_t sub_fmt = *reinterpret_cast<const uint16_t*>(fmt_raw.data() + 24);
                out.format.wFormatTag = sub_fmt; // 1 = PCM, 3 = IEEE_FLOAT
                out.format.cbSize = 0;
            }
            found_fmt = true;


        } else if (std::strncmp(sub_tag, "data", 4) == 0) {
            out.pcm_data.resize(sub_size);
            f.read(reinterpret_cast<char*>(out.pcm_data.data()), sub_size);
            if (aligned > sub_size)
                f.seekg(aligned - sub_size, std::ios::cur);
            found_data = true;

        } else {
            // Skip unknown chunk
            f.seekg(aligned, std::ios::cur);
        }
    }

    return found_fmt && found_data && !out.pcm_data.empty();
}

// -----------------------------------------------------------------------
// Load
// -----------------------------------------------------------------------
bool SoundManager::Load(SoundID id, const std::string& path) {
    const int index = static_cast<int>(id);
    if (index < 0 || index >= SOUND_COUNT) return false;

    WavBuffer& buf = _sounds[index];
    if (!ParseWav(path, buf)) {
        OutputDebugStringA(("[SoundManager] Failed to load: " + path + "\n").c_str());
        return false;
    }

    buf.loaded = true;
    return true;
}

// -----------------------------------------------------------------------
// Play — fire-and-forget source voice
// -----------------------------------------------------------------------
void SoundManager::Play(SoundID id, float volume) {
    if (!_engine) return;

    const int index = static_cast<int>(id);
    if (index < 0 || index >= SOUND_COUNT) return;

    const WavBuffer& buf = _sounds[index];
    if (!buf.loaded) return;

    // Create a fresh source voice for each play (fire-and-forget)
    IXAudio2SourceVoice* voice = nullptr;
    HRESULT hr = _engine->CreateSourceVoice(&voice, &buf.format);
    if (FAILED(hr) || !voice) return;

    voice->SetVolume(volume);

    XAUDIO2_BUFFER xbuf = {};
    xbuf.AudioBytes = static_cast<UINT32>(buf.pcm_data.size());
    xbuf.pAudioData = buf.pcm_data.data();
    xbuf.Flags      = XAUDIO2_END_OF_STREAM;

    hr = voice->SubmitSourceBuffer(&xbuf);
    if (FAILED(hr)) {
        voice->DestroyVoice();
        return;
    }

    voice->Start(0);
    // Voice auto-destroys when playback ends (fire-and-forget)
    // Note: for production, use a callback to recycle voices into a pool
}
