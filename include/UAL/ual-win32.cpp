// UreTech Audio Library Win32 Implementation
#include <UAL/ual.h>
#include <UAL/ual_structs.h>

#include <fstream>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <algorithm>
#include <iostream>


// some utility functions
void writeBufferToFile(void* data, size_t data_size, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
	file.write((const char*)data, data_size);
    file.close();
}

struct wavFileInfo {
    std::vector<int16_t> data;
    size_t numChannels;
	size_t sampleRate;
};

wavFileInfo LoadWavSamples(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) throw std::runtime_error("Can not open WAV file! CODE: 0x00");

    char riff[4];
    file.read(riff, 4);
    if (std::strncmp(riff, "RIFF", 4) != 0) throw std::runtime_error("CODE: 0xA1");

    file.ignore(4);

    char wave[4];
    file.read(wave, 4);
    if (std::strncmp(wave, "WAVE", 4) != 0) throw std::runtime_error("CODE: 0xA2");

    char fmt[4];
    file.read(fmt, 4);
    if (std::strncmp(fmt, "fmt ", 4) != 0) throw std::runtime_error("CODE: 0xA3");

    uint32_t subchunk1Size;
    file.read(reinterpret_cast<char*>(&subchunk1Size), 4);

    uint16_t audioFormat, numChannels, bitsPerSample;
    uint32_t sampleRate, byteRate;
    uint16_t blockAlign;

    file.read(reinterpret_cast<char*>(&audioFormat), 2);
    file.read(reinterpret_cast<char*>(&numChannels), 2);
    file.read(reinterpret_cast<char*>(&sampleRate), 4);
    file.read(reinterpret_cast<char*>(&byteRate), 4);
    file.read(reinterpret_cast<char*>(&blockAlign), 2);
    file.read(reinterpret_cast<char*>(&bitsPerSample), 2);

    if (audioFormat != 1) throw std::runtime_error("Only PCM WAV supported!  CODE: 0xB2");
    if (bitsPerSample != 16) throw std::runtime_error("Only 16-bit WAV format supported! CODE: 0xB3");

    if (subchunk1Size > 16) file.ignore(subchunk1Size - 16);

    char dataHeader[4];
    uint32_t dataSize;

    while (true) {
        file.read(dataHeader, 4);
        file.read((char*) & dataSize, 4);
        if (std::strncmp(dataHeader, "data", 4) == 0) break;
        file.ignore(dataSize);
    }

    size_t sampleCount = dataSize / sizeof(int16_t);
    std::vector<int16_t> samples(sampleCount);

    file.read((char*)samples.data(), dataSize);
    wavFileInfo result;
	result.data = std::move(samples);
	result.numChannels = numChannels;
	result.sampleRate = sampleRate;
	return result;
}

void UAL::ual_wav_to_uad(const char* wavFile, const char* uadFile)
{
	wavFileInfo wavInfo = LoadWavSamples(wavFile);

	// UAD file header
    UAD_Header header;
	header.sample_size = (16 / 8) * wavInfo.numChannels; // UAD only supports 16-bit samples for now (multiplied byt channel count)
	header.sample_rate = wavInfo.sampleRate;
	header.num_channels = wavInfo.numChannels;
    header.num_samples = wavInfo.data.size() / header.sample_size;
	header.data_size = wavInfo.data.size() * sizeof(int16_t);
	header.data_offset = sizeof(UAD_Header);

	uint8_t* rawUADFile = (uint8_t*)malloc(sizeof(UAD_Header) + header.data_size);
	if (!rawUADFile) throw std::runtime_error("Memory allocation failed! CODE: 0x01");

	memcpy(rawUADFile, &header, sizeof(UAD_Header));
	memcpy(rawUADFile + header.data_offset, wavInfo.data.data(), wavInfo.data.size() * sizeof(int16_t));

	writeBufferToFile(rawUADFile, sizeof(UAD_Header) + header.data_size, uadFile);
	free(rawUADFile);
}

UAL::UAL_SampleBuffer UAL::ual_load_uad(const char* uad_file_path)
{
    UAL_SampleBuffer result;
    std::ifstream file(uad_file_path, std::ios::binary);
    if (!file) throw std::runtime_error("Can not open UAD file! CODE: 0x00");


    UAD_Header uadHeader;

	file.read((char*)&uadHeader, sizeof(UAL::UAD_Header));

	result.data_size = uadHeader.data_size;
    result.data = (uint8_t*)malloc(result.data_size);
	result.sample_size = uadHeader.sample_size;
	result.sample_rate = uadHeader.sample_rate;
	result.num_samples = uadHeader.num_samples;
	result.num_channels = uadHeader.num_channels;
	if (!result.data) throw std::runtime_error("Memory allocation failed! CODE: 0x01");

	file.seekg(uadHeader.data_offset, std::ios::beg);
    if (!file) throw std::runtime_error("Can not set offset to data! CODE: 0x02");

	file.read((char*)result.data, result.data_size);
    if (!file) throw std::runtime_error("Can not read data! CODE: 0x03");

    return result;
}

UAL::UAL_SampleBuffer UAL::ual_load_wav(const char* wav_file_path)
{
    UAL_SampleBuffer result;

    wavFileInfo wavInfo = LoadWavSamples(wav_file_path);

	result.sample_size = (16 / 8) * wavInfo.numChannels; // UAD only supports 16-bit samples for now (multiplied by channel count)
    result.sample_rate = wavInfo.sampleRate;
    result.num_channels = wavInfo.numChannels;
    result.num_samples = wavInfo.data.size() / result.sample_size;
	result.data_size = wavInfo.data.size() * sizeof(int16_t);
	result.data = new uint8_t[result.data_size];
	if (!result.data) throw std::runtime_error("Memory allocation failed! CODE: 0x01");
    
    memcpy(result.data, wavInfo.data.data(), result.data_size);

    return result;
}

UAL::UAL_OUTPUT_DEVICE* UAL::ual_initalize_default_audio_device()
{
    UAL_OUTPUT_DEVICE* dev = new UAL_OUTPUT_DEVICE();

    int retCode = CoInitialize(nullptr);
    if (FAILED(retCode)) {
        std::cout << "CoInitialize() fail: " << std::hex << retCode << std::endl;
    }

    retCode = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&dev->pEnumerator);
    if (FAILED(retCode)) {
        std::cout << "CoCreateInstance() fail: " << std::hex << retCode << std::endl;
    }

    retCode = dev->pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &dev->pDevice);
    if (FAILED(retCode)) {
        std::cout << "GetDefaultAudioEndpoint() fail: " << std::hex << retCode << std::endl;
    }

    retCode = dev->pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&dev->pAudioClient);
    if (FAILED(retCode)) {
        std::cout << "Activate() fail: " << std::hex << retCode << std::endl;
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.nSamplesPerSec = 48000;
    waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = (16 / 8) * 2; // 16 bits per sample, 2 channels
    waveFormat.nAvgBytesPerSec = 48000 * ((16 / 8) * 2);

    WAVEFORMATEX* closestMatch = nullptr;
    HRESULT hr = dev->pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &waveFormat, &closestMatch);

   if (hr == S_FALSE) {
        std::cout << "Requested format is not supported!\n";
        if (closestMatch) {
            std::cout << "Closest match sample rate: " << closestMatch->nSamplesPerSec << "Format: " << closestMatch->wFormatTag << std::endl;
            REFERENCE_TIME bufferDuration = 1000000 * 100; // 100ms
            retCode = dev->pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, bufferDuration, 0, closestMatch, nullptr);
            if (FAILED(retCode)) {
                std::cout << "Initialize() fail: " << std::hex << retCode << std::endl;
            }
        }
   }
   else {
       REFERENCE_TIME bufferDuration = 1000000 * 100; // 100ms
       retCode = dev->pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, bufferDuration, 0, &waveFormat, nullptr);
       if (FAILED(retCode)) {
           std::cout << "Initialize() fail: " << std::hex << retCode << std::endl;
       }
   }

    return dev;
}

void UAL::ual_destroy_audio_device(UAL_OUTPUT_DEVICE* device)
{
	// stop the worker thread and clear buffers
    {
        std::lock_guard<std::mutex> lock(device->buffer_mutex);
        device->worker_status = uSTOPPED;
        device->sample_buffers.clear();
    }
    device->pAudioClient->Stop();
    device->pRenderClient->Release();
    device->pAudioClient->Release();
    device->pDevice->Release();
    device->pEnumerator->Release();
    CoUninitialize();
}

void __UAL_STREAM_WORKER(UAL::UAL_OUTPUT_DEVICE* device) {
    uint32_t bufferFrameCount;
    device->pAudioClient->GetBufferSize(&bufferFrameCount);
    int retCode = device->pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&device->pRenderClient);
    if (FAILED(retCode)) {
        std::cout << "GetService() fail: " << std::hex << retCode << std::endl;
    }
    device->pAudioClient->Start();

    const int sleepMs = 5;

    while (true) {
        uint32_t padding = 0;
        device->pAudioClient->GetCurrentPadding( &padding);
        uint64_t availableFrames = bufferFrameCount - padding;
        if (availableFrames == 0) {
            Sleep(10);
            continue;
        }

        unsigned char* pData = nullptr;
        device->pRenderClient->GetBuffer(availableFrames, &pData);

        {
			std::lock_guard<std::mutex> lock(device->buffer_mutex);
            for (uint64_t i = 0; i < availableFrames; ++i) {
                int32_t mixedL = 0, mixedR = 0;
                size_t active_buffers = 0;

                for (uint64_t j = 0; j < device->sample_buffers.size(); j++) {
					UAL::UAL_SampleBuffer* buf = &device->sample_buffers[j];
                    if (buf->sample_counter / buf->num_channels >= buf->num_samples) continue;

                    size_t index = buf->sample_counter * buf->sample_size;
                    if (index + buf->sample_size > buf->data_size) continue;

                    int16_t* samples = reinterpret_cast<int16_t*>(buf->data + index);

                    mixedL += samples[0];
                    mixedR += samples[1];

                    buf->sample_counter++;
                    active_buffers++;

                    if (buf->sample_counter / buf->num_channels >= buf->num_samples) {
                        if (buf->flags & UAL::UAL_SampleBufferFlags::UAL_SBF_LOOP_FLAG_BIT) {
                            buf->sample_counter = 0; // reset counter for looping buffers
                            std::cout << "Sample buffer looped! (" << j << ")" << std::endl;
                        }
                        else {
							device->sample_buffers.erase(device->sample_buffers.begin() + j);
							std::cout << "Sample buffer finished and removed from stream! (" << j << ")" << std::endl;
                        }
                    }
                }

                mixedL = std::clamp(mixedL, -32768, 32767);
                mixedR = std::clamp(mixedR, -32768, 32767);

                *pData++ = mixedL & 0xFF;
                *pData++ = (mixedL >> 8) & 0xFF;
                *pData++ = mixedR & 0xFF;
                *pData++ = (mixedR >> 8) & 0xFF;
            }

            device->pRenderClient->ReleaseBuffer(availableFrames, 0);
            Sleep(sleepMs);
        }
    }
}


void UAL::ual_start_audio_stream(UAL_OUTPUT_DEVICE* device)
{
	std::thread streamWorker(__UAL_STREAM_WORKER, device);
	device->worker_status = uWORKING;
	streamWorker.detach();
}

void UAL::ual_add_sample_to_stream(UAL_OUTPUT_DEVICE* device, UAL_SampleBuffer sampleBuffer)
{
    {
		std::lock_guard<std::mutex> lock(device->buffer_mutex);
        device->sample_buffers.push_back(sampleBuffer);
    }
}
