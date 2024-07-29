#pragma once

#include <SDL.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include <math.h>
#include "AudioFile.h"

#include "stb_image.h"
#include "stb_image_write.h"

using namespace std;

const int sFreq = 48000;

// encoder
#define AUDIO_RATE 12288
#define AUDIO_SCAN_LEN 512
#define SAMPLES_INFRAME 4000
#define SYNC_HI 4000
#define SYNC_LO -4000

const int audio_peak = 600;
const int signal_peak = 600;
const int chroma_signal_peak = 600;

const double chroma_amp = 1;
const double luma_amp = 1;

const int output_signal_amp = 6;
const int frame_w = 85;
const int scan_w = 100;
const int chroma_w = 42;
const int frame_h = 64;

// playback
const Uint8 CCV_PLAYBACK = 0;
const Uint8 CCI_DISPLAY = 1;

const Sint16 cci_sync_high = 15000;
const Sint16 cci_sync_low = -15000;

const int luma_level = 25; // lower is more
const int chroma_level = 35; // lower is more
const double sync_detect_sensitivity = 2.2; // 1<  // higher is more sensitive
const unsigned int adaptive_sync_interval = 4000 * 5;
const int offset = 11;
const int audio_scan_offset = 30;
const int chroma_offset = 0;
const int sync_delay = 40; // amount of samples to block sync trigger
const int MAX_RECORDING_DEVICES = 10;
const int SAMPLE_SIZE = 2048;
const int screen_w = 800;
const int screen_h = 600;
const int SCALE = 5;

//Recording data buffer
struct AudioBuffer{
    Uint8* buffer;
    Uint32 size;
    Uint32 front_pos;
    Uint32 back_pos;
    Uint8 BytesInSample;
};

struct AudioStitch
{
    Sint16 data[AUDIO_SCAN_LEN];
    unsigned int pos = 0;
};

// adaptive sync threshold
struct adaptiveSync{
    Sint32 max = 1000;
    Sint32 min = -1000;
    Sint32 sync_high;
    Sint32 sync_low;
};

const double signal_amp = (double)signal_peak / 127.0; // -127 to 127 * signal_amp
const double chroma_signal_amp = (double)chroma_signal_peak / 127.0;
