
#include "audiosys/audio_manager.hpp"
#include <stb_vorbis.c>
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"

namespace mam {

	bool isValidIndex(int index, std::size_t size) {
		return index >= 0 && static_cast<std::size_t>(index) < size;
	}

	void clearOpenALError() {
		while (alGetError() != AL_NO_ERROR) {}
	}

	bool checkOpenALError(const char* message) {
		const ALenum error = alGetError();
		if (error == AL_NO_ERROR) {
			return true;
		}

		std::cerr << message << " OpenAL error: " << error << '\n';
		return false;
	}

	ALenum resolveOpenALFormat(int channels, int bitsPerSample) {
		if (channels == 1) {
			return bitsPerSample == 8 ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
		}
		if (channels == 2) {
			return bitsPerSample == 8 ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
		}
		return 0;
	}

  AudioManager::AudioManager(){
		device = nullptr;
		ctx = nullptr;

		src.clear();

		size = 0;
		frequency = 0;
		format = 0;

		masterGain = 1.0f;

		for (int i = 0; i < kAudioLayerCount; ++i) {
			layerGains[i] = 1.0f;
		}
	}

  AudioManager::~AudioManager(){
		for (auto& source : src) {
			if (source.source != 0) {
				alSourceStop(source.source);
				alSourcei(source.source, AL_BUFFER, 0);
				alDeleteSources(1, &source.source);
				source.source = 0;
			}
		}

		for (auto& audioBuffer : buffer) {
			if (audioBuffer != 0) {
				alDeleteBuffers(1, &audioBuffer);
				audioBuffer = 0;
			}
		}

		src.clear();
		buffer.clear();

		if (ctx != nullptr) {
			alcMakeContextCurrent(nullptr);
			alcDestroyContext(ctx);
			ctx = nullptr;
		}

		if (device != nullptr) {
			alcCloseDevice(device);
			device = nullptr;
		}
	
	}

	bool AudioManager::LoadWavFile(const std::string filename, int index_buffer) {
		if (index_buffer < 0) return false;
		if (static_cast<std::size_t>(index_buffer) >= buffer.size()) {
			buffer.resize(static_cast<std::size_t>(index_buffer) + 1, 0);
		}

		FILE* soundFile = nullptr;
		fopen_s(&soundFile, filename.c_str(), "rb");
		if (!soundFile) {
			std::cerr << "AudioManager::LoadWavFile failed to open: " << filename << '\n';
			return false;
		}

		RIFF_Header riffHeader{};
		if (fread(&riffHeader, sizeof(RIFF_Header), 1, soundFile) != 1) {
			fclose(soundFile);
			return false;
		}
		if (std::memcmp(riffHeader.chunkID, "RIFF", 4) != 0 ||
			std::memcmp(riffHeader.format, "WAVE", 4) != 0) {
			fclose(soundFile);
			return false;
		}

		long fmtSize = 0;
		if (!FindNextChunk(soundFile, "fmt ", fmtSize)) {
			fclose(soundFile);
			return false;
		}

		WAVE_Format waveFormat{};
		if (fread(&waveFormat, sizeof(WAVE_Format), 1, soundFile) != 1) {
			fclose(soundFile);
			return false;
		}

		if (fmtSize > 16) {
			fseek(soundFile, fmtSize - 16, SEEK_CUR);
		}

		if (waveFormat.audioFormat != 1) {
			fclose(soundFile);
			return false;
		}

		long dataSize = 0;
		if (!FindNextChunk(soundFile, "data", dataSize) || dataSize <= 0) {
			fclose(soundFile);
			return false;
		}

		std::vector<unsigned char> audioData(static_cast<std::size_t>(dataSize));
		if (fread(audioData.data(), static_cast<std::size_t>(dataSize), 1, soundFile) != 1) {
			fclose(soundFile);
			return false;
		}

		fclose(soundFile);
		const ALenum decodedFormat = resolveOpenALFormat(
			waveFormat.numChannels,waveFormat.bitsPerSample);
		if (decodedFormat == 0) {
			std::cerr << "AudioManager::LoadWavFile unsupported WAV format: " << filename << '\n';
			return false;
		}

		if (buffer[index_buffer] != 0) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
		}

		clearOpenALError();
		alGenBuffers(1, &buffer[index_buffer]);
		if (!checkOpenALError("AudioManager::LoadWavFile failed creating buffer.")) {
			return false;
		}

		alBufferData(buffer[index_buffer],decodedFormat,audioData.data(),
			static_cast<ALsizei>(audioData.size()),static_cast<ALsizei>(waveFormat.sampleRate));
		if (!checkOpenALError("AudioManager::LoadWavFile failed uploading buffer.")) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
			return false;
		}

		size = static_cast<ALsizei>(audioData.size());
		frequency = static_cast<ALsizei>(waveFormat.sampleRate);
		format = decodedFormat;
		return true;
	}

	bool AudioManager::LoadOGGFile(const std::string filename, int index_buffer) {
		if (index_buffer < 0) return false;
		if (static_cast<std::size_t>(index_buffer) >= buffer.size()) {
			buffer.resize(static_cast<std::size_t>(index_buffer) + 1, 0);
		}

		int channels = 0;
		int sampleRate = 0;
		short* rawOutput = nullptr;
		const int samples = stb_vorbis_decode_filename(filename.c_str(),
			&channels,&sampleRate,&rawOutput);
		std::unique_ptr<short, decltype(&std::free)> output(rawOutput, &std::free);
		if (samples < 0 || output == nullptr) {
			std::cerr << "AudioManager::LoadOGGFile failed decoding: " << filename << '\n';
			return false;
		}

		const ALenum decodedFormat = channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		const ALsizei dataSize = static_cast<ALsizei>(samples * channels * sizeof(short));
		if (buffer[index_buffer] != 0) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
		}

		clearOpenALError();
		alGenBuffers(1, &buffer[index_buffer]);
		if (!checkOpenALError("AudioManager::LoadOGGFile failed creating buffer.")) {
			return false;
		}

		alBufferData(buffer[index_buffer], decodedFormat, output.get(), dataSize, sampleRate);
		if (!checkOpenALError("AudioManager::LoadOGGFile failed uploading buffer.")) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
			return false;
		}

		size = dataSize;
		frequency = sampleRate;
		format = decodedFormat;
		return true;
	}

	bool AudioManager::LoadMP3File(const std::string filename, int index_buffer) {
		if (index_buffer < 0) return false;
		if (static_cast<std::size_t>(index_buffer) >= buffer.size()) {
			buffer.resize(static_cast<std::size_t>(index_buffer) + 1, 0);
		}

		drmp3_config config{};
		drmp3_uint64 totalFrameCount = 0;
		drmp3_int16* sampleData = drmp3_open_file_and_read_pcm_frames_s16(
			filename.c_str(),	&config,&totalFrameCount,nullptr);

		auto deleter = [](drmp3_int16* ptr) {
			if (ptr != nullptr) drmp3_free(ptr, nullptr);
		};

		std::unique_ptr<drmp3_int16, decltype(deleter)> samples(sampleData, deleter);
		if (samples == nullptr) {
			std::cerr << "AudioManager::LoadMP3File failed decoding: " << filename << '\n';
			return false;
		}

		const ALenum decodedFormat = config.channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		const ALsizei dataSize = static_cast<ALsizei>(
			totalFrameCount * config.channels * sizeof(drmp3_int16));

		if (buffer[index_buffer] != 0) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
		}

		clearOpenALError();
		alGenBuffers(1, &buffer[index_buffer]);
		if (!checkOpenALError("AudioManager::LoadMP3File failed creating buffer.")) {
			return false;
		}

		alBufferData(buffer[index_buffer],decodedFormat,samples.get(),
			dataSize,static_cast<ALsizei>(config.sampleRate));

		if (!checkOpenALError("AudioManager::LoadMP3File failed uploading buffer.")) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
			return false;
		}

		size = dataSize;
		frequency = static_cast<ALsizei>(config.sampleRate);
		format = decodedFormat;
		return true;
	}

	bool AudioManager::LoadFLACFile(const std::string filename, int index_buffer) {
		if (index_buffer < 0) return false;
		if (static_cast<std::size_t>(index_buffer) >= buffer.size()) {
			buffer.resize(static_cast<std::size_t>(index_buffer) + 1, 0);
		}
		unsigned int channels = 0;
		unsigned int sampleRate = 0;
		drflac_uint64 totalSampleCount = 0;
		drflac_int16* sampleData = drflac_open_file_and_read_pcm_frames_s16(
			filename.c_str(),	&channels,
			&sampleRate, &totalSampleCount,nullptr);

		auto deleter = [](drflac_int16* ptr) {
			if (ptr != nullptr) drflac_free(ptr, nullptr);
		};

		std::unique_ptr<drflac_int16, decltype(deleter)> samples(sampleData, deleter);
		if (samples == nullptr) {
			std::cerr << "AudioManager::LoadFLACFile failed decoding: " << filename << '\n';
			return false;
		}

		const ALenum decodedFormat = channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
		const ALsizei dataSize = static_cast<ALsizei>(
			totalSampleCount * channels * sizeof(drflac_int16));

		if (buffer[index_buffer] != 0) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
		}

		clearOpenALError();
		alGenBuffers(1, &buffer[index_buffer]);
		if (!checkOpenALError("AudioManager::LoadFLACFile failed creating buffer.")) {
			return false;
		}

		alBufferData(buffer[index_buffer],decodedFormat,
			samples.get(),dataSize,static_cast<ALsizei>(sampleRate));

		if (!checkOpenALError("AudioManager::LoadFLACFile failed uploading buffer.")) {
			alDeleteBuffers(1, &buffer[index_buffer]);
			buffer[index_buffer] = 0;
			return false;
		}

		size = dataSize;
		frequency = static_cast<ALsizei>(sampleRate);
		format = decodedFormat;
		return true;
	}

	void AudioManager::CreateContext() {
		if (ctx != nullptr) return;
		device = alcOpenDevice(nullptr);
		if (device == nullptr) {
			std::cerr << "AudioManager::CreateContext failed opening OpenAL device.\n";
			return;
		}
		ctx = alcCreateContext(device, nullptr);
		if (ctx == nullptr) {
			std::cerr << "AudioManager::CreateContext failed creating OpenAL context.\n";
			alcCloseDevice(device);
			device = nullptr;
			return;
		}
		if (alcMakeContextCurrent(ctx) == ALC_FALSE) {
			std::cerr << "AudioManager::CreateContext failed making context current.\n";
			alcDestroyContext(ctx);
			alcCloseDevice(device);
			ctx = nullptr;
			device = nullptr;
			return;
		}
		alDopplerFactor(1.0f);
		alDopplerVelocity(343.3f);
	}

	void AudioManager::CreateSource(SourceData source) {
		source.source = 0;
		src.push_back(source);
		//buffer.push_back(0);
		clearOpenALError();
		alGenSources(1, &src.back().source);
		if (!checkOpenALError("AudioManager::CreateSource failed creating source.")) {
			src.pop_back();
			buffer.pop_back();
			return;
		}

		const ALfloat sourcePos[] = {
			src.back().position.x,
			src.back().position.y,
			src.back().position.z
		};

		const ALfloat sourceVel[] = {
			src.back().velocity.x,
			src.back().velocity.y,
			src.back().velocity.z
		};

		alSourcef(src.back().source, AL_PITCH, src.back().pitch);
		alSourcef(src.back().source, AL_GAIN, src.back().gain);
		alSourcefv(src.back().source, AL_POSITION, sourcePos);
		alSourcefv(src.back().source, AL_VELOCITY, sourceVel);
		alSourcei(src.back().source, AL_LOOPING, src.back().loop ? AL_TRUE : AL_FALSE);

		UpdateSourceGains();
	}

	void AudioManager::PlaySource(int index) {
		if (index < 0) return;
		if (index >= static_cast<int>(src.size())) return;
		if (index >= static_cast<int>(buffer.size())) return;
		if (buffer[index] == 0) return;

		alSourcei(src[index].source, AL_BUFFER, buffer[index]);
		alSourcePlay(src[index].source);
	}

	void AudioManager::PauseSource(int index) {
		if (!isValidIndex(index, src.size())) return;
		alSourcePause(src[index].source);
	}

	void AudioManager::StopSource(int index) {
		if (!isValidIndex(index, src.size()))	return;
		alSourceStop(src[index].source);
	}

	bool AudioManager::IsSourcePlaying(int index) {
		if (!isValidIndex(index, src.size()))	return false;
		ALint state = 0;
		alGetSourcei(src[index].source, AL_SOURCE_STATE, &state);
		return state == AL_PLAYING;
	}

	void AudioManager::SetSourcePitch(int index, float pitch) {
		if (!isValidIndex(index, src.size())) return;
		src[index].pitch = pitch;
		alSourcef(src[index].source, AL_PITCH, pitch);
	}

	void AudioManager::SetSourceGain(int index, float gain) {
		if (!isValidIndex(index, src.size())) return;
		src[index].gain = glm::clamp(gain, 0.0f, 1.0f);
		UpdateSourceGains();
	}

	void AudioManager::SetSourcePosition(int index, const glm::vec3& pos) {
		if (!isValidIndex(index, src.size()))	return;
		src[index].position = pos;
		const ALfloat sourcePos[] = { pos.x, pos.y, pos.z };
		alSourcefv(src[index].source, AL_POSITION, sourcePos);
	}

	void AudioManager::SetSourceVelocity(int index, const glm::vec3& vel) {
		if (!isValidIndex(index, src.size())) return;
		src[index].velocity = vel;
		const ALfloat sourceVel[] = { vel.x, vel.y, vel.z };
		alSourcefv(src[index].source, AL_VELOCITY, sourceVel);
	}

	float AudioManager::GetSourcePitch(int index) const {
		if (!isValidIndex(index, src.size()))
			return 1.0f;
		return src[index].pitch;
	}

	float AudioManager::GetSourceGain(int index) const {
		if (!isValidIndex(index, src.size()))
			return 1.0f;
		return src[index].gain;
	}

	glm::vec3 AudioManager::GetSourcePosition(int index) const {
		if (!isValidIndex(index, src.size())) 
			return glm::vec3(0.0f);
		return src[index].position;
	}

	glm::vec3 AudioManager::GetSourceVelocity(int index) const {
		if (!isValidIndex(index, src.size()))
			return glm::vec3(0.0f);
		return src[index].velocity;
	}

	void AudioManager::SetListener() {
		alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
		alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
	}

	void AudioManager::SetListener(const glm::vec3& pos, const glm::vec3& front, const glm::vec3& up) {
		alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
		alListener3f(AL_VELOCITY, 0.0f, 0.0f, 0.0f);
		const ALfloat orientation[6] = {
			front.x, front.y, front.z,
			up.x,    up.y,    up.z
		};

		alListenerfv(AL_ORIENTATION, orientation);
	}

#pragma region INTERNAL
	bool AudioManager::FindNextChunk(FILE* f, const char target[4], long& outSize) {
		if (f == nullptr) return false;
		while (true) {
			char chunkId[4]{};
			long chunkSize = 0;

			if (fread(chunkId, 1, 4, f) != 4) return false;
			if (fread(&chunkSize, sizeof(long), 1, f) != 1) return false;

			if (std::memcmp(chunkId, target, 4) == 0) {
				outSize = chunkSize;
				return true;
			}
			if (chunkSize <= 0 || fseek(f, chunkSize, SEEK_CUR) != 0)
				return false;
		}
	}
#pragma endregion

	void AudioManager::SetMasterGain(float gain) {
		masterGain = glm::clamp(gain, 0.0f, 1.0f);
		UpdateSourceGains();
	}

	void AudioManager::SetLayerGain(AudioLayer layer, float gain) {
		const int index = static_cast<int>(layer);
		if (index < 0 || index >= kAudioLayerCount) return;

		this->layerGains[index] = glm::clamp(gain, 0.0f, 1.0f);
		UpdateSourceGains();
	}

	float AudioManager::GetLayerGain(AudioLayer layer) const {
		const int index = static_cast<int>(layer);

		if (index < 0 || index >= kAudioLayerCount)	return 1.0f;
		return this->layerGains[index];
	}

	void AudioManager::UpdateSourceGains() {
		for (auto& source : src) {
			const int layerIndex = static_cast<int>(source.layer);

			const float layerGain =
				layerIndex >= 0 && layerIndex < kAudioLayerCount
				? this->layerGains[layerIndex]
				: 1.0f;

			const float finalGain = masterGain * layerGain * source.gain;
			alSourcef(source.source, AL_GAIN, finalGain);
		}
	}

}
