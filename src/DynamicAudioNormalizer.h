/* ================================================================================== */
/* Dynamic Audio Normalizer - Audio Processing Library                                */
/* Copyright (c) 2014-2019 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.             */
/*                                                                                    */
/* This library is free software; you can redistribute it and/or                      */
/* modify it under the terms of the GNU Lesser General Public                         */
/* License as published by the Free Software Foundation; either                       */
/* version 2.1 of the License, or (at your option) any later version.                 */
/*                                                                                    */
/* This library is distributed in the hope that it will be useful,                    */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of                     */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                  */
/* Lesser General Public License for more details.                                    */
/*                                                                                    */
/* You should have received a copy of the GNU Lesser General Public                   */
/* License along with this library; if not, write to the Free Software                */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA     */
/*                                                                                    */
/* http://www.gnu.org/licenses/lgpl-2.1.txt                                           */
/* ================================================================================== */

#pragma once

/*StdLib includes*/
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif
#include <stdint.h>

/*Utility macros*/
#define MDYNAMICAUDIONORMALIZER_GLUE1(X,Y) X##Y
#define MDYNAMICAUDIONORMALIZER_GLUE2(X,Y) MDYNAMICAUDIONORMALIZER_GLUE1(X,Y)

/*Interface version*/
#define MDYNAMICAUDIONORMALIZER_CORE 8
#define MDYNAMICAUDIONORMALIZER_FUNCTION(X) MDYNAMICAUDIONORMALIZER_GLUE2(MDynamicAudioNormalizer_##X##_r, MDYNAMICAUDIONORMALIZER_CORE)
#define MDynamicAudioNormalizer MDYNAMICAUDIONORMALIZER_GLUE2(MDynamicAudioNormalizer_r,MDYNAMICAUDIONORMALIZER_CORE)

#include "Common.h"

/*Callback for log messages*/
typedef void (MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction))(const int logLevel, const char *const message);

/* ================================================================================== */
/* API for C++ application                                                            */
/* ================================================================================== */

#ifdef __cplusplus /*C++ compiler!*/

/*Standard Library include*/
#include <cstdio>

/*Opaque Data Class*/
class MDynamicAudioNormalizer_PrivateData;

/*Dynamic Normalizer Class*/
class MDynamicAudioNormalizer
{
public:
	/*Constructor & Destructor*/
	MDynamicAudioNormalizer(const uint32_t channels, const uint32_t sampleRate, const uint32_t frameLenMsec = 500, const uint32_t filterSize = 31, const double peakValue = 0.95, const double maxAmplification = 10.0, const double targetRms = 0.0, const double compressFactor = 0.0, const bool channelsCoupled = true, const bool enableDCCorrection = false, const bool altBoundaryMode = false, FILE *const logFile = NULL);
	virtual ~MDynamicAudioNormalizer(void);
	
	/*Public API*/
	bool initialize(void);
	bool process(const DAN_Data *const *const samplesIn, DAN_Data *const *const samplesOut, const int64_t inputSize, int64_t &outputSize);
	bool processInplace(DAN_Data *const *const samplesInOut, const int64_t inputSize, int64_t &outputSize);
	bool flushBuffer(DAN_Data *const *const samplesOut, const int64_t bufferSize, int64_t &outputSize);
	bool reset(void);
	bool getConfiguration(uint32_t &channels, uint32_t &sampleRate, uint32_t &frameLen, uint32_t &filterSize);
	bool getInternalDelay(int64_t &delayInSamples);

	/*Type definitions*/
	typedef MDYNAMICAUDIONORMALIZER_FUNCTION(LogFunction) LogFunction;
	enum { LOG_LEVEL_DBG = 0, LOG_LEVEL_WRN = 1, LOG_LEVEL_ERR = 2 };

	/*Static functions*/
	static void getVersionInfo(uint32_t &major, uint32_t &minor, uint32_t &patch);
	static void getBuildInfo(const char **const date, const char **const time, const char **const compiler, const char **const arch, bool &debug);
	static LogFunction *setLogFunction(LogFunction *const logFunction);

private:
	MDynamicAudioNormalizer(const MDynamicAudioNormalizer&) : p(NULL)  { throw "unsupported"; }
	MDynamicAudioNormalizer &operator=(const MDynamicAudioNormalizer&) { throw "unsupported"; }

	MDynamicAudioNormalizer_PrivateData *const p;
};

#endif /*__cplusplus*/

