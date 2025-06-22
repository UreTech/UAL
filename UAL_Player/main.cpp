#define UAL_IMPORT_FROM_DLL
#include <UAL/ual.h>
#include <exception>
#include <iostream>

using namespace UAL;

int main() {
	try {

		//ual_wav_to_uad("test.wav", "test.uad");

		UAL_OUTPUT_DEVICE* device = ual_initalize_default_audio_device();

		ual_start_audio_stream(device);

		UAL_SampleBuffer test = ual_load_wav("test.wav");
		UAL_ADD_FLAG(test.flags, UAL_SBF_LOOP_FLAG_BIT);
		
		ual_add_sample_to_stream(device, test);

		while (1);

		ual_destroy_audio_device(device);

	} catch (const std::exception& e) {
		std::cerr << "An error occurred: " << e.what() << std::endl;
		return 1;
	}
}