#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL.h>
#include <cstdio>
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <ctime>
#include <math.h>

// encoder
#define AUDIO_RATE 19200
#define SAMPLES_INFRAME 4000
#define SYNC_HI 4000
#define SYNC_LO -4000

const int audio_peak = 600;
const int signal_peak = 500;

const int output_signal_amp = 5;
const int frame_w = 85;
const int scan_w = 100;
const int chroma_w = 42;
const int frame_h = 64;

// playback
bool invert_signal = false;
const int luma_level = 80; // lower is more
const int chroma_level = 80; // lower is more
const double sync_detect_sensitivity = 1.2; // 1 <
const int offset = 10;
const int sync_delay = 15;
const int MAX_RECORDING_DEVICES = 10;
const int SAMPLE_SIZE = 1024;
const int sFreq = 48000;
const int screen_w = 640;
const int screen_h = 480;
const int SCALE = 4;

int xCord = 0;
int yCord = 0;
int interlace = 0;
Sint16 hi_sync_threshold = 0;
Sint16 lo_sync_threshold = 0;
bool update_display = true;
bool new_data_in_buffer = false;
int sync_block_delay = 0;
//Recieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;
//Recording data buffer
Uint8* gRecordingBuffer = NULL;
//Size of data buffer
Uint32 gBufferByteSize = 0;
//Position in data buffer
Uint32 gBufferBytePosition = 0;
//Maximum position in data buffer for recording
Uint32 gBufferByteMaxPosition = 0;
int gRecordingDeviceCount = 0;
SDL_PixelFormat *fmt;
Sint16 chan1, chan2;

const double signal_amp = signal_peak / 127.0; // -127 to 127 * signal_amp
using namespace std;

#endif
