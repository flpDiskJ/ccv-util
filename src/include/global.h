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
bool invert_signal = true;
bool swap_endianess = false;
bool swap_channels = false;
const int luma_level = 60; // lower is more
const int chroma_level = 60; // lower is more
const double sync_detect_sensitivity = 2.5; // 1<  // higher is more sensitive
const int offset = 11;
const int chroma_offset = 1;
const int sync_delay = 15;
const int MAX_RECORDING_DEVICES = 10;
const int SAMPLE_SIZE = 512;
const int sFreq = 48000;
const int screen_w = 800;
const int screen_h = 600;
const int SCALE = 6;

int luma_data[scan_w][frame_h];
int chroma_u_data[scan_w][frame_h];
int chroma_v_data[scan_w][frame_h];
int xCord = 0;
int yCord = 0;
int interlace = 0;
Sint16 hi_sync_threshold = 0;
Sint16 lo_sync_threshold = 0;
bool update_display = true;
int sync_block_delay = 0;

//Recieved audio spec
SDL_AudioSpec gReceivedRecordingSpec;

//Recording data buffer
struct AudioBuffer{
    Uint8* buffer;
    Uint32 size;
    Uint32 front_pos;
    Uint32 back_pos;
    Uint8 BytesInSample;
};
int gRecordingDeviceCount = 0;
SDL_PixelFormat *fmt;
Sint16 chan1, chan2;

const double signal_amp = signal_peak / 127.0; // -127 to 127 * signal_amp
using namespace std;

#endif
