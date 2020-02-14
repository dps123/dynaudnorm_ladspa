/* Copyright 2020 Pavel Demenkov

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#include <cstdlib>
#include <cstring>
#include <algorithm>

#include <ladspa.h>
#include "DynamicAudioNormalizer.h"

#define DAN_FRAMELEN        0
#define DAN_GAUSSSIZE       1
#define DAN_PEAK            2
#define DAN_MAXGAIN         3
#define DAN_TARGETRMS       4
#define DAN_COMPRESS        5
#define DAN_COUPLING        6
#define DAN_CORRECTDC       7
#define DAN_ALTBOUNDARY     8

#define DAN_AUDIO_INPUT1   9
#define DAN_AUDIO_OUTPUT1  10
#define DAN_AUDIO_INPUT2   11
#define DAN_AUDIO_OUTPUT2  12
#define DAN_AUDIO_INPUT3   13
#define DAN_AUDIO_OUTPUT3  14
#define DAN_AUDIO_INPUT4   15
#define DAN_AUDIO_OUTPUT4  16
#define DAN_AUDIO_INPUT5   17
#define DAN_AUDIO_OUTPUT5  18
#define DAN_AUDIO_INPUT6   19
#define DAN_AUDIO_OUTPUT6  20
#define DAN_AUDIO_INPUT7   21
#define DAN_AUDIO_OUTPUT7  22
#define DAN_AUDIO_INPUT8   23
#define DAN_AUDIO_OUTPUT8  24


#define INT2BOOL(X) ((X) != 0)
#define FLOAT2BOOL(X) ((X) > 0)

static LADSPA_Descriptor *dynaudnormDescriptors[8] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static const char * labels[] = { "dynaudnorm_mono", "dynaudnorm_stereo", "dynaudnorm_21", "dynaudnorm_40", "dynaudnorm_41", "dynaudnorm_51", "dynaudnorm_61", "dynaudnorm_71"};

typedef struct {
	LADSPA_Data **input;
	LADSPA_Data **output;

	uint32_t channels;
	uint32_t sampleRate;
	uint32_t frameLenMsec;
	uint32_t filterSize;
	double peakValue;
	double maxAmplification;
	double targetRms;
	double compressFactor;
	bool channelsCoupled;
	bool enableDCCorrection;
	bool altBoundaryMode;
	MDynamicAudioNormalizer *dan;
} Dynaudnorm;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index) {
	if (index<8) {
		return dynaudnormDescriptors[index];
	} else {
		return NULL;
	}
}

static LADSPA_Handle instantiateDynaudnorm(
 const LADSPA_Descriptor *descriptor,
 unsigned long s_rate) {
	Dynaudnorm *plugin_data = (Dynaudnorm *)std::calloc(1, sizeof(Dynaudnorm));
	plugin_data->input = (LADSPA_Data **) std::malloc(8*sizeof(LADSPA_Data *));
	plugin_data->output = (LADSPA_Data **) std::malloc(8*sizeof(LADSPA_Data *));
	plugin_data->sampleRate = s_rate;

	// DEFAULT_VALUES
	plugin_data->frameLenMsec = 500;
	plugin_data->filterSize = 31;
	plugin_data->peakValue = 0.95;
	plugin_data->maxAmplification = 10.0;
	plugin_data->targetRms = 0.0;
	plugin_data->compressFactor = 0.0;
	plugin_data->channelsCoupled = true;
	plugin_data->enableDCCorrection = false;
	plugin_data->altBoundaryMode = false;
	
	return (LADSPA_Handle)plugin_data;
}

static void connectPortDynaudnorm(
 LADSPA_Handle instance,
 unsigned long port,
 LADSPA_Data *data) {
	Dynaudnorm *plugin;

	plugin = (Dynaudnorm *)instance;
	switch (port) {
	case DAN_FRAMELEN:
		plugin->frameLenMsec = (uint32_t)data[0];
		break;
	case DAN_GAUSSSIZE:
		plugin->filterSize = (uint32_t)data[0];
		break;
	case DAN_PEAK:
		plugin->peakValue = data[0];
		break;
	case DAN_MAXGAIN:
		plugin->maxAmplification = data[0];
		break;
	case DAN_TARGETRMS:
		plugin->targetRms = data[0];
		break;
	case DAN_COMPRESS:
		plugin->compressFactor = data[0];
		break;
	case DAN_COUPLING:
		plugin->channelsCoupled = FLOAT2BOOL(data[0]);
		break;
	case DAN_CORRECTDC:
		plugin->enableDCCorrection = FLOAT2BOOL(data[0]);
		break;
	case DAN_ALTBOUNDARY:
		plugin->altBoundaryMode = FLOAT2BOOL(data[0]);
		break;
	default:
		if (port>=DAN_AUDIO_INPUT1 && port<=DAN_AUDIO_OUTPUT8){
			if (DAN_AUDIO_INPUT1 % 2 == port % 2) {
				plugin->input[(port-DAN_AUDIO_INPUT1) / 2] = data;
				if (data!=NULL)
					plugin->channels = std::max(plugin->channels, ((uint32_t)port-DAN_AUDIO_INPUT1) / 2 + 1);
			} else {
				plugin->output[(port-DAN_AUDIO_OUTPUT1) / 2] = data;
			}
			
		}
	}
}
/*
static void Log(const int logLevel, const char *const message){
	switch(logLevel) {
		case 0:
			printf("DEBUG\t%s\n", message);
			break;
		case 1:
			printf("WARN\t%s\n", message);
			break;
		case 2:
			printf("ERROR\t%s\n", message);
			break;
	}
}
*/
static void activateDynaudnorm(LADSPA_Handle instance) {
	Dynaudnorm *plugin_data = (Dynaudnorm *)instance;
	plugin_data->dan = new MDynamicAudioNormalizer(
						plugin_data->channels, plugin_data->sampleRate, plugin_data->frameLenMsec,
						plugin_data->filterSize, plugin_data->peakValue, plugin_data->maxAmplification,
						plugin_data->targetRms, plugin_data->compressFactor, plugin_data->channelsCoupled,
						plugin_data->enableDCCorrection, plugin_data->altBoundaryMode, NULL);
	//plugin_data->dan->setLogFunction(Log);
	plugin_data->dan->initialize();

	int64_t delayInSamples;
	int64_t out_count;
	if (plugin_data->dan->getInternalDelay(delayInSamples)) {
		LADSPA_Data **input = (LADSPA_Data **) std::malloc(plugin_data->channels*sizeof(LADSPA_Data *));
		for (uint32_t k = 0; k < plugin_data->channels; k++) {
			input[k] = (LADSPA_Data *) std::calloc(delayInSamples, sizeof(LADSPA_Data));
		}

		plugin_data->dan->processInplace(input, delayInSamples, out_count);

		for (uint32_t k = 0; k < plugin_data->channels; k++) {
			std::free((LADSPA_Data *)input[k]);
		}
		std::free((LADSPA_Data **)input);
	}
}

static void deactivateDynaudnorm(LADSPA_Handle instance) {
	Dynaudnorm *plugin_data = (Dynaudnorm *)instance;
	delete(plugin_data->dan);
}

static void cleanupDynaudnorm(LADSPA_Handle instance) {
	Dynaudnorm *plugin_data = (Dynaudnorm *)instance;
	std::free(plugin_data->input);
	std::free(plugin_data->output);
	std::free(instance);
}

static void runDynaudnorm(LADSPA_Handle instance, unsigned long sample_count) {
	Dynaudnorm *plugin_data = (Dynaudnorm *)instance;
	int64_t in_count = sample_count;
	int64_t out_count;
	plugin_data->dan->process(plugin_data->input, plugin_data->output, in_count, out_count);
}

static void fillDescriptor(LADSPA_Descriptor * descriptor, int channels, const char *label){

	char **port_names;
	LADSPA_PortDescriptor *port_descriptors;
	LADSPA_PortRangeHint *port_range_hints;
		
	descriptor->UniqueID = 9900+channels;
	descriptor->Label = label;
	descriptor->Properties =
	 LADSPA_PROPERTY_HARD_RT_CAPABLE;
	descriptor->Name = (char *)"Dynamic Audio Normalizer";
	descriptor->Maker = (char *)"Pavel Demenkov <demenkovps@gmail.com>";
	descriptor->Copyright = (char *)"GPL";
	descriptor->PortCount = 9 + 2 * channels;

	port_descriptors = (LADSPA_PortDescriptor *)std::calloc(descriptor->PortCount, sizeof(LADSPA_PortDescriptor));
	descriptor->PortDescriptors = (const LADSPA_PortDescriptor *)port_descriptors;

	port_range_hints = (LADSPA_PortRangeHint *)std::calloc(descriptor->PortCount, sizeof(LADSPA_PortRangeHint));
	descriptor->PortRangeHints = (const LADSPA_PortRangeHint *)port_range_hints;

	port_names = (char **)std::calloc(descriptor->PortCount, sizeof(char*));
	descriptor->PortNames = (const char **)port_names;

	/* Parameters for the frame length in msec */
	port_descriptors[DAN_FRAMELEN] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_FRAMELEN] = (char *)"set the frame length in msec";
	port_range_hints[DAN_FRAMELEN].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
	port_range_hints[DAN_FRAMELEN].LowerBound = (LADSPA_Data) 10;
	port_range_hints[DAN_FRAMELEN].UpperBound = (LADSPA_Data) 8000;

	/* Parameters for the filter size */
	port_descriptors[DAN_GAUSSSIZE] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_GAUSSSIZE] = (char *)"set the filter size";
	port_range_hints[DAN_GAUSSSIZE].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
	port_range_hints[DAN_GAUSSSIZE].LowerBound = (LADSPA_Data) 3;
	port_range_hints[DAN_GAUSSSIZE].UpperBound = (LADSPA_Data) 301;

	/* Parameters for the peak value */
	port_descriptors[DAN_PEAK] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_PEAK] = (char *)"set the peak value";
	port_range_hints[DAN_PEAK].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_HIGH;
	port_range_hints[DAN_PEAK].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_PEAK].UpperBound = (LADSPA_Data) 1.0;

	/* Parameters for the max amplification */
	port_descriptors[DAN_MAXGAIN] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_MAXGAIN] = (char *)"set the max amplification";
	port_range_hints[DAN_MAXGAIN].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE;
	port_range_hints[DAN_MAXGAIN].LowerBound = (LADSPA_Data) 1.0;
	port_range_hints[DAN_MAXGAIN].UpperBound = (LADSPA_Data) 100.0;

	/* Parameters for the target RMS */
	port_descriptors[DAN_TARGETRMS] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_TARGETRMS] = (char *)"set the target RMS";
	port_range_hints[DAN_TARGETRMS].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
	port_range_hints[DAN_TARGETRMS].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_TARGETRMS].UpperBound = (LADSPA_Data) 1.0;

	/* Parameters for the compress factor */
	port_descriptors[DAN_COMPRESS] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_COMPRESS] = (char *)"set the compress factor";
	port_range_hints[DAN_COMPRESS].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
	port_range_hints[DAN_COMPRESS].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_COMPRESS].UpperBound = (LADSPA_Data) 30.0;

	/* Parameters for the channel coupling */
	port_descriptors[DAN_COUPLING] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_COUPLING] = (char *)"set the channel coupling";
	port_range_hints[DAN_COUPLING].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM;
	port_range_hints[DAN_COUPLING].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_COUPLING].UpperBound = (LADSPA_Data) 1.0;

	/* Parameters for DC correction */
	port_descriptors[DAN_CORRECTDC] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_CORRECTDC] = (char *)"set DC correction";
	port_range_hints[DAN_CORRECTDC].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
	port_range_hints[DAN_CORRECTDC].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_CORRECTDC].UpperBound = (LADSPA_Data) 1.0;

	/* Parameters for alternative boundary mode */
	port_descriptors[DAN_ALTBOUNDARY] =
	 LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[DAN_ALTBOUNDARY] = (char *)"set alternative boundary mode";
	port_range_hints[DAN_ALTBOUNDARY].HintDescriptor =
	 LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM;
	port_range_hints[DAN_ALTBOUNDARY].LowerBound = (LADSPA_Data) 0.0;
	port_range_hints[DAN_ALTBOUNDARY].UpperBound = (LADSPA_Data) 1.0;
	
	#define snprintf_nowarn(...) (snprintf(__VA_ARGS__) < 0 ? abort() : (void)0)
	for (int i = 0; i < channels; i++){

		/* Parameters for Input */
		port_descriptors[DAN_AUDIO_INPUT1 + 2*i] = LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO;
		//port_names[DAN_AUDIO_INPUT1 + 2*i] = (char *)"Input";
		port_names[DAN_AUDIO_INPUT1 + 2*i] = (char *) std::calloc(8, sizeof(char));
		snprintf_nowarn(port_names[DAN_AUDIO_INPUT1 + 2*i], 8, "Input%-u", i);
		port_range_hints[DAN_AUDIO_INPUT1 + 2*i].HintDescriptor = LADSPA_HINT_DEFAULT_NONE;

		/* Parameters for Output */
		port_descriptors[DAN_AUDIO_OUTPUT1 + 2*i] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
		//port_names[DAN_AUDIO_OUTPUT1 + 2*i] = (char *)"Output";
		port_names[DAN_AUDIO_OUTPUT1 + 2*i] = (char *) std::calloc(9, sizeof(char));
		snprintf_nowarn(port_names[DAN_AUDIO_OUTPUT1 + 2*i], 9, "Output%-u", i);
		port_range_hints[DAN_AUDIO_OUTPUT1 + 2*i].HintDescriptor = LADSPA_HINT_DEFAULT_NONE;
	}

	descriptor->activate = activateDynaudnorm;
	descriptor->cleanup = cleanupDynaudnorm;
	descriptor->connect_port = connectPortDynaudnorm;
	descriptor->deactivate = deactivateDynaudnorm;
	descriptor->instantiate = instantiateDynaudnorm;
	descriptor->run = runDynaudnorm;
	descriptor->run_adding = NULL;
	descriptor->set_run_adding_gain = NULL;
}


static void __attribute__((constructor)) _init(void) {
	for (uint32_t i = 0; i < 8; i++) {
		dynaudnormDescriptors[i] = (LADSPA_Descriptor *)std::malloc(sizeof(LADSPA_Descriptor));
		if (dynaudnormDescriptors[i]) {
			fillDescriptor(dynaudnormDescriptors[i], i+1, labels[i]);
		}
	}
}

static void __attribute__((destructor)) _fini(void) {
	for (uint32_t i = 0; i < 8; i++) {
		LADSPA_Descriptor *descriptor = dynaudnormDescriptors[i];
		if (descriptor) {
			std::free((LADSPA_PortDescriptor *)descriptor->PortDescriptors);
			for (uint32_t k = DAN_AUDIO_INPUT1; k < descriptor->PortCount; k++){
				std::free((char *)descriptor->PortNames[k]);
			}
			std::free((char **)descriptor->PortNames);
			std::free((LADSPA_PortRangeHint *)descriptor->PortRangeHints);
			std::free((LADSPA_Descriptor *)descriptor);
		}
	}
}

