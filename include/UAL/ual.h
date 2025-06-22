#ifdef UAL_IMPORT_FROM_DLL
#define UAL_API __declspec(dllimport)
#else
#define UAL_API __declspec(dllexport)

#endif

#ifndef UAL_H
#define UAL_H

#define UAL_PLATFORM "win32"
#define UAL_VERSION_MAJOR 1
#define UAL_VERSION_MINOR 0
#define UAL_VERSION_PATCH 0
#define UAD_VERSION_MAJOR 1
#define UAD_VERSION_MINOR 0
#define UAD_VERSION_PATCH 0

#define UAL_ADD_FLAG(var, flag) ((var) |= (flag))

#include <UAL/ual_structs.h>

namespace UAL
{
	// WAV to UAD 
	UAL_API void ual_wav_to_uad(const char* wavFile, const char* uadFile);

	// UAD to playable sample buffer
	UAL_API UAL_SampleBuffer ual_load_uad(const char* uad_file_path);

	// WAV to playable sample buffer
	UAL_API UAL_SampleBuffer ual_load_wav(const char* wav_file_path);

	// Intalize UAL
	UAL_API UAL_OUTPUT_DEVICE* ual_initalize_default_audio_device();

	// Destroy UAL
	UAL_API void ual_destroy_audio_device(UAL_OUTPUT_DEVICE* device);

	// Start audio stream
	UAL_API void ual_start_audio_stream(UAL_OUTPUT_DEVICE* device);

	// Add sample to audio stream
	UAL_API void ual_add_sample_to_stream(UAL_OUTPUT_DEVICE* device, UAL_SampleBuffer sampleBuffer);
}

#endif