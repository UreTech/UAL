#ifndef UAL_STRUCTS_H
#define UAL_STRUCTS_H

#include <UAL/ual.h>

#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

namespace UAL
{

	struct UAD_Header {
		uint8_t uad_ver_major = UAD_VERSION_MAJOR;
		uint8_t uad_ver_minor = UAD_VERSION_MINOR;
		uint8_t uad_ver_PATCH = UAD_VERSION_PATCH;
		size_t sample_size = 0;
		size_t sample_rate = 0;
		size_t num_channels = 0;
		size_t num_samples = 0;
		uint64_t data_offset = 0;
		uint64_t data_size = 0;
	};

	typedef enum {
		UAL_SBF_NO_FLAG          = 0b00000000'00000000,
		UAL_SBF_LOOP_FLAG_BIT    = 0b00000000'00000001,
		//UAL_SBF_REVERSE_FLAG_BIT = 0b00000000'00000010,
	}UAL_SampleBufferFlags;

	struct UAL_SampleBuffer {
		uint8_t* data = nullptr;
		uint64_t data_size = 0;
		uint32_t flags = UAL_SBF_NO_FLAG;
		size_t sample_size = 0;
		size_t sample_rate = 0;
		size_t num_channels = 0;
		size_t num_samples = 0;
		uint64_t sample_counter = 0;
	};

	typedef enum{
	uERROR   = -1,
	uWORKING = 0,
	uSTOPPED = 1,
	}UAL_STREAM_WORKER_STATUS;

	struct UAL_OUTPUT_DEVICE {
		IMMDeviceEnumerator* pEnumerator = nullptr;
		IMMDevice* pDevice = nullptr;
		IAudioClient* pAudioClient = nullptr;
		IAudioRenderClient* pRenderClient = nullptr;

		UAL_STREAM_WORKER_STATUS worker_status = uSTOPPED;

		std::mutex buffer_mutex;
		std::vector<UAL_SampleBuffer> sample_buffers;
	};
}

#endif // !UAL_STRUCTS_H

