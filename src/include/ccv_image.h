#pragma once

#include "global.h"

class ccv_image{
private:
    vector<string> files;
    vector<Sint16> channel_1, channel_2;
    Sint16 modulation_freq[5] = {0, 3, 3, -3, -3}; // used to modulate luma signal
    Uint8 mod_pos = 0;
    const int frame_width = 750;
    const int frame_height = 500;
    const Sint16 sync_high = 800;
    const Sint16 sync_low = -800; // realative to 128 <-> -127
    const int signal_multiplier = 40;

    void convert_image(int i);

    void make_wav();

    void push_modulation(Sint16 val); // modulate val and push to channel_2

    void stitch(Sint16 *u, Sint16 *v, Sint16 *l);

    void blank(int sNum);

    void sync_x();

    void sync_y();

public:

    ccv_image();

    ~ccv_image();

};
