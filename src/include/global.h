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
double sound_level = 2;
bool invert_signal = true;
bool swap_endianess = false;
bool swap_channels = false;
const int luma_level = 20; // lower is more
const int chroma_level = 25; // lower is more
const double sync_detect_sensitivity = 2.2; // 1<  // higher is more sensitive
const unsigned int adaptive_sync_interval = 4000 * 5;
const int offset = 11;
const int audio_scan_offset = 30;
const int chroma_offset = 0;
const int sync_delay = 15;
const int MAX_RECORDING_DEVICES = 10;
const int SAMPLE_SIZE = 2048;
const int sFreq = 48000;
const int screen_w = 800;
const int screen_h = 600;
const int SCALE = 5;

int luma_data[scan_w][frame_h];
int chroma_u_data[scan_w][frame_h];
int chroma_v_data[scan_w][frame_h];
int xCord = 0;
int yCord = 0;
int interlace = 0;
bool update_display = true;
int sync_block_delay = 0;

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
AudioStitch c1_buffer, c2_buffer;

SDL_PixelFormat *fmt;
Sint16 chan1, chan2;
float chan1_level, chan2_level;
signed int audio_scan_pos = AUDIO_SCAN_LEN;

// adaptive sync threshold
struct adaptiveSync{
    Sint32 max = 1;
    Sint32 min = -1;
    Sint32 sync_high;
    Sint32 sync_low;
};
adaptiveSync sync_1, sync_2;
unsigned int tick_count = 0;

const double signal_amp = (double)signal_peak / 127.0; // -127 to 127 * signal_amp
const double chroma_signal_amp = (double)chroma_signal_peak / 127.0;
using namespace std;

#endif
