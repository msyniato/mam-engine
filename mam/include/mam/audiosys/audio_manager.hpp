#pragma once

#include "common/common.hpp"

namespace mam {

  /**
   * @brief Represents the RIFF header of a WAV file.
   */
  struct RIFF_Header {
    char chunkID[4];      /**< Identifier of the chunk ("RIFF"). */
    long chunkSize;       /**< Size of the file excluding chunkID and chunkSize. */
    char format[4];       /**< Format type, usually "WAVE". */
  };

  /**
   * @brief Represents the WAVE format subchunk of a WAV file.
   */
  struct WAVE_Format {
    short audioFormat;    /**< Audio format (1 = PCM). */
    short numChannels;    /**< Number of audio channels. */
    long sampleRate;      /**< Samples per second. */
    long byteRate;        /**< Bytes per second. */
    short blockAlign;     /**< Bytes per sample slice. */
    short bitsPerSample;  /**< Bits per audio sample. */
  };

  /**
   * @brief Represents the data subchunk of a WAV file.
   */
  struct WAVE_Data {
    char subChunkID[4];     /**< Identifier of the data chunk ("data"). */
    long subChunk2Size;     /**< Size of the audio data block. */
  };

  /**
   * @brief Available audio layers for volume grouping.
   */
  enum AudioLayer {
    MASTER = 0,   /**< Global/master layer. */
    MUSIC = 1,    /**< Music layer. */
    SFX = 2,      /**< Sound effects layer. */
    AMBIENT = 3,  /**< Ambient sounds layer. */
    UI = 4        /**< User interface sounds layer. */
  };

  /**
   * @brief Holds parameters for an OpenAL audio source.
   */
  struct SourceData {
    float pitch;            /**< Playback pitch. */
    float gain;             /**< Base gain of the source (before layer/master modifiers). */
    glm::vec3 position;     /**< 3D position of the source. */
    glm::vec3 velocity;     /**< 3D velocity of the source. */
    ALuint source;          /**< OpenAL source ID. */
    bool loop;              /**< Whether the source loops playback. */

    AudioLayer layer;       /**< Audio layer this source belongs to. */
    ALuint buffer;          /**< OpenAL buffer ID. */
  };

  /**
   * @brief Manages audio device, context, buffers, sources, and layer-based volume.
   */
  class AudioManager {

  public:
    /**
     * @brief Constructs the audio manager and initializes internal values.
     */
    AudioManager();

    /**
     * @brief Destroys the audio manager and releases OpenAL resources.
     */
    ~AudioManager();

    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;
    AudioManager(AudioManager&&) noexcept = delete;
    AudioManager& operator=(AudioManager&&) noexcept = delete;

    /**
     * @brief Loads a WAV file into the specified buffer index.
     * @param filename Path to the audio file.
     * @param index_buffer Target buffer index.
     * @return True on success, false otherwise.
     */
    bool LoadWavFile(const std::string filename, int index_buffer);

    /**
     * @brief Loads an OGG file into the specified buffer index.
     */
    bool LoadOGGFile(const std::string filename, int index_buffer);

    /**
     * @brief Loads an MP3 file into the specified buffer index.
     */
    bool LoadMP3File(const std::string filename, int index_buffer);

    /**
     * @brief Loads a FLAC file into the specified buffer index.
     */
    bool LoadFLACFile(const std::string filename, int index_buffer);

    /**
     * @brief Creates an OpenAL context and initializes the audio device.
     */
    void CreateContext();

    /**
     * @brief Creates an audio source with the given parameters.
     * @param source Source configuration data.
     */
    void CreateSource(SourceData source);

    /**
     * @brief Plays the source at the given index.
     */
    void PlaySource(int index);

    /**
     * @brief Pauses the source at the given index.
     */
    void PauseSource(int index);

    /**
     * @brief Stops the source at the given index.
     */
    void StopSource(int index);

    /**
     * @brief Checks whether the specified source is currently playing.
     * @return True if playing, false otherwise.
     */
    bool IsSourcePlaying(int index);

    /**
     * @brief Sets the pitch of a source.
     */
    void SetSourcePitch(int index, float pitch);

    /**
     * @brief Sets the gain of a source (before layer/master mixing).
     */
    void SetSourceGain(int index, float gain);

    /**
     * @brief Sets the 3D position of a source.
     */
    void SetSourcePosition(int index, const glm::vec3& pos);

    /**
     * @brief Sets the 3D velocity of a source.
     */
    void SetSourceVelocity(int index, const glm::vec3& vel);

    /**
     * @brief Gets the pitch of a source.
     */
    float GetSourcePitch(int index) const;

    /**
     * @brief Gets the base gain of a source.
     */
    float GetSourceGain(int index) const;

    /**
     * @brief Gets the 3D position of a source.
     */
    glm::vec3 GetSourcePosition(int index) const;

    /**
     * @brief Gets the 3D velocity of a source.
     */
    glm::vec3 GetSourceVelocity(int index) const;

    /**
     * @brief Returns the number of created sources.
     */
    int GetSourceCount() const { return (int)src.size(); }

    /**
     * @brief Updates listener position and orientation using default values.
     */
    void SetListener();

    /**
     * @brief Updates listener position and orientation.
     */
    void SetListener(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up);

    // --- LAYERING ---

    /**
     * @brief Sets the global (master) gain applied to all audio.
     */
    void SetMasterGain(float gain);

    /**
     * @brief Sets the gain for a specific audio layer.
     */
    void SetLayerGain(AudioLayer layer, float gain);

    /**
     * @brief Gets the master gain.
     */
    float GetMasterGain() const {
      return masterGain;
    }

    /**
     * @brief Gets the gain applied to a specific audio layer.
     */
    float GetLayerGain(AudioLayer layer) const;

    /**
     * @brief Recalculates all source gains based on layer and master gains.
     */
    void UpdateSourceGains();

  protected:

    /**
     * @brief Searches for a specific chunk in a WAV file by ID.
     * @param f File pointer.
     * @param target Chunk ID to locate.
     * @param outSize Output chunk size.
     * @return True if found, false otherwise.
     */
    static bool FindNextChunk(FILE* f, const char target[4], long& outSize);

    std::vector<ALuint> buffer;            /**< OpenAL buffers storing audio data. */
    std::vector<SourceData> src;           /**< List of created audio sources. */

    ALCdevice* device;                     /**< OpenAL audio device. */
    ALCcontext* ctx;                       /**< OpenAL audio context. */

    ALsizei size;                          /**< Loaded audio size in bytes. */
    ALsizei frequency;                     /**< Audio sample rate. */
    ALenum format;                         /**< Audio format identifier. */

    float masterGain;                      /**< Global gain applied to all audio. */

    static constexpr int kAudioLayerCount = static_cast<int>(AudioLayer::UI) + 1; ///< Number of audio layers.
    float layerGains[kAudioLayerCount];              ///< Gain multiplier per audio layer.
  };

}
