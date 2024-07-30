#pragma once

#include "global.h"

class ccv_2{
private:
    vector<Sint16> channel_1, channel_2;
    vector<Sint16> luma, chroma_u, chroma_v, chroma_u_downscale, chroma_v_downscale;
    const int frame_width = 120;
    const int frame_height = 90;
    bool y_offset = false;
    int y_start = 0; // set by sync_y()
    int zFill;
    Sint16 *audio;
    Uint32 audio_length = 0;
    Uint32 audio_pos = 0;

    void convert_image(string filename);

    void make_wav();

    bool load_audio();

    void chroma_downscale();

    void blank(int sNum);

    void sync_x();

    void sync_audio();

    void sync_y(bool offset);

    string fill(int num);

public:

    ccv_2();

    ~ccv_2();

};
