// Code by Jake Aigner December 2023
// Functional: March, 2024

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "include/ccv_image.h"
#include "include/ccv2.h"

double sound_level = 2;
bool invert_signal = true;
bool swap_endianess = false;
bool swap_channels = false;
int luma_data[120][90];
int chroma_u_data[120][90];
int chroma_v_data[120][90];
vector<int> ccv2_chroma_u, ccv2_chroma_v;
int xCord = 0;
int yCord = 0;
int interlace = 0;
bool update_display = true;
int sync_block_delay = 0;

Uint8 display_mode = CCV_PLAYBACK;

int cci_u[750], cci_v[750], cci_l[1500];

AudioStitch c1_buffer, c2_buffer;

SDL_PixelFormat *fmt;
Sint16 chan1, chan2;
float chan1_level, chan2_level;
signed int audio_scan_pos = AUDIO_SCAN_LEN;

adaptiveSync sync_1, sync_2;
unsigned int tick_count = 0;

class Encoder
{
private:
    int *channel1;
    int *channel2;
    Uint32 signal_length = 0;
    Uint32 chan1_pos = 48000;
    Uint32 chan2_pos = 48000;
    int total_frames = 0;
    int frame_pos = 0;
    int zFill;
    int *audio; // audio -4k to 4k
    Uint32 audio_length = 0;
    Uint32 audio_pos = 0;
    unsigned char *img = NULL;

    string fill(int num)
    {
        int power;
        string tx = "";
        for (int z = zFill - 1; z > 0; z--)
        {
            power = pow(10, z);
            if (num < power)
            {
                tx += "0";
            }
        }
        tx += to_string(num);
        return tx;
    }

    bool load_audio()
    {
        SDL_AudioSpec inputSpec;
        Uint8 *buffer;
        Uint32 buffer_length;
        if (SDL_LoadWAV("input/audio.wav", &inputSpec, &buffer, &buffer_length) == NULL)
        {
            fprintf(stderr, "Could not open wav: %s\n", SDL_GetError());
            return false;
        }
        // convert input format
        if (inputSpec.format == AUDIO_S16)
        {
            Sint16 val;
            Sint16 val1;
            double pos = 0;
            int actual_pos = 0;
            double adv = (double)AUDIO_RATE / (double)inputSpec.freq;
            if (inputSpec.channels == 1)
            {
                audio_length = (buffer_length * adv) + 100;
                audio = (int*)malloc(sizeof(int)*audio_length);
                memset(audio, 0, audio_length);
                for (int x = 0; x < buffer_length; x += 2)
                {
                    val = ((buffer[x+1] & 0xFF) << 8) | (buffer[x] & 0xFF);
                    actual_pos = (int)pos;
                    audio[actual_pos] = val;
                    pos += adv;
                }
            } else
            {
                audio_length = ((buffer_length / 2) * adv) + 100;
                audio = (int*)malloc(sizeof(int)*audio_length);
                memset(audio, 0, audio_length);
                for (int x = 0; x < buffer_length; x += 4)
                {
                    val = ((buffer[x+1] & 0xFF) << 8) | (buffer[x] & 0xFF);
                    val1 = ((buffer[x+3] & 0xFF) << 8) | (buffer[x+2] & 0xFF);
                    actual_pos = (int)pos;
                    audio[actual_pos] = (val + val1) / 2;
                    pos += adv;
                }
            }
            SDL_FreeWAV(buffer);
        } else {
            printf("Invalid Audio Format! Must be Signed 16bit!\n");
            SDL_FreeWAV(buffer);
            return false;
        }
        // Find audio peak
        int peak = 0;
        for (int x = 0; x < audio_length; x++)
        {
            if (audio[x] > peak)
            {peak = audio[x];}
        }
        // convert range
        double amp = peak / (double)audio_peak;
        for (int x = 0; x < audio_length; x++)
        {
            audio[x] = (int)(audio[x] / amp);
        }
        return true;
    }

    void blank(int chan, int amount)
    {
        if (chan == 1)
        {
            for (int s = 0; s < amount; s++, chan1_pos++)
            {
                channel1[chan1_pos] = 0;
            }
        }
        else if (chan == 2)
        {
            for (int s = 0; s < amount; s++, chan2_pos++)
            {
                channel2[chan2_pos] = 0;
            }
        } else {
            printf("Blank Func Syntax Error!\n");
        }
    }

    void sync_x() // 10s
    {
        for (int s = 0; s < 5; s++, chan1_pos++)
        {
            channel1[chan1_pos] = SYNC_HI;
        }
        for (int s = 0; s < 5; s++, chan1_pos++)
        {
            channel1[chan1_pos] = SYNC_LO;
        }
    }

    void sync_y() // 10s
    {
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_HI;
        }
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_LO;
        }
    }

    void sync_y_offset() // 10s
    {
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_LO;
        }
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_HI;
        }
    }

    void audio_sync() // 10s
    {
        for (int s = 0; s < 5; s++, chan1_pos++)
        {
            channel1[chan1_pos] = SYNC_LO;
        }
        for (int s = 0; s < 5; s++, chan1_pos++)
        {
            channel1[chan1_pos] = SYNC_HI;
        }
        blank(2, 10);
    }

    void write_wav()
    {
        AudioFile<int> audioFile;
        // Setup audioFile specs
        audioFile.setAudioBufferSize (2, signal_length);
        audioFile.setBitDepth (16);
        audioFile.setSampleRate (sFreq);
        for (int x = 0; x < signal_length; x++)
        {
            audioFile.samples[0][x] = channel1[x] * output_signal_amp;
            audioFile.samples[1][x] = channel2[x] * output_signal_amp;
        }
        audioFile.save ("output/CCV_signal.wav", AudioFileFormat::Wave);
    }

public:
    Encoder()
    {
        // User Input
        printf("Start frame (without z fill): ");
        scanf("%d", &frame_pos);
        printf("End frame (without z fill): ");
        scanf("%d", &total_frames);
        printf("Amount of z fill: ");
        scanf("%d", &zFill);
        printf("%d - %d\n", frame_pos, total_frames);
        signal_length = ((total_frames - frame_pos) * SAMPLES_INFRAME) + 96000;
        channel1 = (int*)malloc(sizeof(int)*signal_length);
        channel2 = (int*)malloc(sizeof(int)*signal_length);
        memset(channel1, 0, signal_length);
        memset(channel2, 0, signal_length);

        // load and convert audio
        if (load_audio() == false)
        {
            return;
        }

        // load frames and generate signal
        string path;
        unsigned char r, g, b; // RGB Pixel values
        int l, u, v; // YUV Pixel values
        int width, height, channels;
        int y_offset = 0;
        int u_buff[frame_w];
        int v_buff[frame_w];
        int l_buff[frame_w];
        for (int f = frame_pos; f <= total_frames; f++)
        {
            path.clear();
            path += "input/";
            path += fill(f);
            path += ".png";
            if (img != NULL)
            {
                free(img);
                img = NULL;
            }
            img = stbi_load(path.c_str(), &width, &height, &channels, 3);
            if (img == NULL)
            {
                printf("Couldn't load frame %s\n", path.c_str());
                return;
            }
            if (width != frame_w)
            {
                printf("Frame width is %d and must be 85!\n", width);
                return;
            }
            if (height != frame_h)
            {
                printf("Frame height is %d and must be 64!\n", height);
                return;
            }

            for (int y = y_offset; y < frame_h; y += 2)
            {
                if (y == 0)
                {
                    sync_y(); blank(1, 10);
                } else if (y == 1)
                {
                    sync_y_offset(); blank(1, 10);
                } else {
                    blank(2, 10);
                    sync_x();
                }
                for (int x = 0; x < frame_w; x++)
                {
                    r = img[3 * ((y*frame_w) + x)    ];
                    g = img[3 * ((y*frame_w) + x) + 1];
                    b = img[3 * ((y*frame_w) + x) + 2];
                    l = 0.299*r + 0.587*g + 0.114*b;
                    u = -0.147*r - 0.289*g + 0.436*b;
                    v = 0.615*r - 0.515*g - 0.100*b;
                    if (l > 255){l = 255;}
                    if (l < 0){l = 0;}
                    if (u > 127){u = 127;}
                    if (u < -128){u = -128;}
                    if (v > 127){v = 127;}
                    if (v < -128){v = -128;}
                    u_buff[x] = u * chroma_signal_amp;
                    v_buff[x] = v * chroma_signal_amp;
                    l_buff[x] = (l-128) * signal_amp;
                } // x loop

                for (int p = 0; p < frame_w; p++, chan2_pos++)
                {
                    channel2[chan2_pos] = l_buff[p];
                }
                blank(2, 5);

                blank(1, 3);
                for (int p = 0, rs = 0; p < chroma_w; p++, chan1_pos++, rs += 2)
                {
                    channel1[chan1_pos] = (u_buff[rs] + u_buff[rs+1]) / 2;
                }
                for (int p = 0, rs = 0; p < chroma_w; p++, chan1_pos++, rs += 2)
                {
                    channel1[chan1_pos] = (v_buff[rs] + v_buff[rs+1]) / 2;
                }
                blank(1, 3);
            } // y loop
            y_offset = !y_offset;

            // Audio
            audio_sync();
            blank(1, 10);
            blank(2, 10);
            for (int a = 0; a < 10; a++, chan1_pos++)
            {
                channel1[chan1_pos] = audio[audio_pos];
            }
            for (int a = 0; a < 10; a++, chan2_pos++)
            {
                channel2[chan2_pos] = audio[audio_pos];
            }

            // actual audio
            for (int a = 0; a < AUDIO_SCAN_LEN; a++, audio_pos++, chan1_pos++)
            {
                channel1[chan1_pos] = audio[audio_pos];
            }
            for (int a = 0; a < AUDIO_SCAN_LEN; a++, audio_pos++, chan2_pos++)
            {
                channel2[chan2_pos] = audio[audio_pos];
            }

            for (int a = 0; a < 10; a++, chan1_pos++)
            {
                channel1[chan1_pos] = audio[audio_pos];
            }
            for (int a = 0; a < 10; a++, chan2_pos++)
            {
                channel2[chan2_pos] = audio[audio_pos];
            }
            blank(1, 760-AUDIO_SCAN_LEN);
            blank(2, 760-AUDIO_SCAN_LEN);
        } // frame loop
        write_wav();
    }

    ~Encoder()
    {
        if (img != NULL)
        {
            free(img);
        }
        free(channel1);
        free(channel2);
        free(audio);
    }

};

void audioRecordingCallback(void* userdata, Uint8* stream, int len)
{
    AudioBuffer *b = (AudioBuffer*)userdata;

    int region1size = len;
    int region2size = 0;

    if (b->front_pos + len >= b->size)
    {
        region1size = b->size - b->front_pos;
        region2size = len - region1size;
    }

    SDL_memcpy(
    &b->buffer[b->front_pos],
    stream,
    region1size
    );
    SDL_memcpy(
    b->buffer,
    &stream[region1size],
    region2size
    );

    b->front_pos = (b->front_pos + len) % b->size;
}

void audioPlaybackCallback(void* userdata, Uint8* stream, int len)
{
    AudioBuffer *b = (AudioBuffer*)userdata;

    int region1size = len;
    int region2size = 0;

    if (b->back_pos + len > b->size)
    {
        region1size = b->size - b->back_pos;
        region2size = len - region1size;
    }

    SDL_memcpy(
    stream,
    &b->buffer[b->back_pos],
    region1size
    );
    SDL_memcpy(
    &stream[region1size],
    b->buffer,
    region2size
    );

    memset(&b->buffer[b->back_pos], 0, region1size);
    memset(b->buffer, 0, region2size);

    b->back_pos = (b->back_pos + len) % b->size;
}

int clip(int val)
{
    int out = val;
    if (out > 255)
    {out = 255;}
    else if (out < 0)
    {out = 0;}
    return out;
}

void clear_buffs()
{
    for (int y = 0; y < frame_h; y++)
    {
        for (int x = 0; x < scan_w; x++)
        {
            luma_data[x][y] = 0;
            chroma_u_data[x][y] = 0;
            chroma_v_data[x][y] = 0;
        }
    }
}

void draw(SDL_Texture *tex)
{
    int u_line[scan_w];
    int v_line[scan_w];
    vector<int> chroma_u_upscale, chroma_v_upscale; // ccv2
    memset(u_line, 0, scan_w);
    memset(v_line, 0, scan_w);
    int r, g, b, l, u, v;
    void *pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, &pixels, &pitch);
    if (display_mode == CCV_PLAYBACK)
    {
        for (int y = 0; y < frame_h; y++)
        {
            for (int ln = chroma_offset, ad = 0; ln < chroma_offset+(frame_w/2); ln++, ad += 2)
            {
                u_line[ad] = chroma_u_data[ln][y];
                u_line[ad+1] = chroma_u_data[ln][y];
                v_line[ad] = chroma_v_data[ln][y];
                v_line[ad+1] = chroma_v_data[ln][y];
            }
            for (int x = 0; x < frame_w-1; x++)
            {
                l = luma_data[x+offset][y];
                u = u_line[x];
                v = v_line[x];
                r = l + 1.140*v;
                g = l - 0.395*u - 0.581*v;
                b = l + 2.032*u;
                r = clip(r);
                g = clip(g);
                b = clip(b);
                ((Uint32*)pixels)[ x + (y * frame_w) ] = SDL_MapRGB(fmt, b, g, r);
            }
        }}
    else
    {
        // upscale chroma data from 1800 to 10800
        for (int c = 0; c < 900; c++)
        {
            for (int i = 0; i < 12; i++)
            {
                if (c < ccv2_chroma_u.size() && c < ccv2_chroma_v.size())
                {
                    chroma_u_upscale.push_back(ccv2_chroma_u[c]);
                    chroma_v_upscale.push_back(ccv2_chroma_v[c]);
                } else {
                    chroma_u_upscale.push_back(0);
                    chroma_v_upscale.push_back(0);
                }
            }
        }
        // draw frame
        int chroma_position = 0;
        for (int y = 0; y < 90; y++)
        {
            for (int x = 0; x < 120; x++)
            {
                l = luma_data[x][y];
                u = chroma_u_upscale[chroma_position];
                v = chroma_v_upscale[chroma_position++];
                r = l + 1.140*v;
                g = l - 0.395*u - 0.581*v;
                b = l + 2.032*u;
                r = clip(r);
                g = clip(g);
                b = clip(b);
                ((Uint32*)pixels)[ x + (y * 120) ] = SDL_MapRGB(fmt, b, g, r);
            }
        }
    }
    SDL_UnlockTexture(tex);
    chroma_u_upscale.clear();
    chroma_v_upscale.clear();
    ccv2_chroma_u.clear();
    ccv2_chroma_v.clear();
}

Sint16 clip16(Sint32 input)
{
    if (input > 32766)
    {
        return 32766;
    } else if (input < -32766)
    {
        return -32766;
    } else {
        return (Sint16)input;
    }
}

void stitch_audio(Sint16 *output)
{
    unsigned int pos = 0;
    int length;
    if (display_mode == CCV_PLAYBACK)
    {
        length = AUDIO_SCAN_LEN;
    } else {
        length = CCV2_AUDIO_SCAN_LEN;
    }
    for (int x = 0; x < length; x++)
    {
        output[pos++] = c1_buffer.data[x];
    }
    for (int x = 0; x < length; x++)
    {
        output[pos++] = c2_buffer.data[x];
    }
    memset(&c1_buffer.data[0], 0, length);
    memset(&c2_buffer.data[0], 0, length);
}

void set_sync_threshold()
{
    sync_1.sync_high = sync_1.max / sync_detect_sensitivity;
    sync_1.sync_low = sync_1.min / sync_detect_sensitivity;
    sync_1.max = 1000;
    sync_1.min = -1000;
    sync_2.sync_high = sync_2.max / sync_detect_sensitivity;
    sync_2.sync_low = sync_2.min / sync_detect_sensitivity;
    sync_2.max = 1000;
    sync_2.min = -1000;
}

void draw_cci_line(SDL_Texture *tex, int y)
{
    void *pixels;
    int pitch;
    int pos = 0;
    int r, g, b, l, u, v;
    SDL_LockTexture(tex, NULL, &pixels, &pitch);
    for (int x = 0; x < 1500; x += 2)
    {
        l = (cci_l[x] + cci_l[x+1]) / 2;
        u = cci_u[pos];
        v = cci_v[pos];
        r = l + 1.140*v;
        g = l - 0.395*u - 0.581*v;
        b = l + 2.032*u;
        r = clip(r);
        g = clip(g);
        b = clip(b);
        r = g = b = 100;
        ((Uint32*)pixels)[ pos + (y * 750) ] = SDL_MapRGB(fmt, b, g, r);
        pos++;
    }
    SDL_UnlockTexture(tex);
}

void scanner(AudioBuffer *b, AudioBuffer *plybck, SDL_Texture *cci_tex)
{
    Sint32 level_check1, level_check2; // used for normalization
    while (b->back_pos != b->front_pos)
    {
        if (swap_endianess)
        {
            chan2 = ((b->buffer[b->back_pos] & 0xFF) << 8) | (b->buffer[b->back_pos+1] & 0xFF);
            chan1 = ((b->buffer[b->back_pos+2] & 0xFF) << 8) | (b->buffer[b->back_pos+3] & 0xFF);
            level_check1 = (Sint32)chan1;
            level_check2 = (Sint32)chan2;
        } else {
            chan1 = ((b->buffer[b->back_pos+1] & 0xFF) << 8) | (b->buffer[b->back_pos] & 0xFF);
            chan2 = ((b->buffer[b->back_pos+3] & 0xFF) << 8) | (b->buffer[b->back_pos+2] & 0xFF);
            level_check1 = (Sint32)chan1;
            level_check2 = (Sint32)chan2;
        }
        if (swap_channels)
        {
            Sint16 temp;
            temp = chan1;
            chan1 = chan2;
            chan2 = temp;
        }
        if (invert_signal)
        {
            chan1 = 0 - chan1;
            chan2 = 0 - chan2;
        }

        chan1 = clip16(chan1 * chan1_level);
        chan2 = clip16(chan2 * chan2_level);

        if (display_mode == CCV_PLAYBACK)
        {

            if (chan2 > sync_2.sync_high && sync_block_delay == 0)
            {
                yCord = 0;
                interlace = 0;
                xCord = 0;
                update_display = true;
                sync_block_delay = sync_delay;
            }
            if (chan2 < sync_2.sync_low && sync_block_delay == 0)
            {
                yCord = 1;
                interlace = 1;
                xCord = 0;
                update_display = true;
                sync_block_delay = sync_delay;
            }
            if (chan1 > sync_1.sync_high && sync_block_delay == 0)
            {
                xCord = 0;
                yCord += 2;
                sync_block_delay = sync_delay;
                if (yCord >= frame_h)
                {
                    update_display = true;
                    yCord = interlace = !interlace;
                }
            }
            if (chan1 < sync_1.sync_low && sync_block_delay == 0)
            {
                audio_scan_pos = 0 - audio_scan_offset;
                c1_buffer.pos = 0;
                c2_buffer.pos = 0;
                sync_block_delay = sync_delay;
                int actual_pos = 0;
                double stitch_pos = 0;
                double adv = (double)AUDIO_RATE / (double)sFreq;
                Sint16 full_audio[AUDIO_SCAN_LEN*2];
                stitch_audio(&full_audio[0]);
                while (plybck->front_pos != plybck->back_pos && actual_pos < 1024)
                {
                    plybck->buffer[plybck->front_pos] = full_audio[actual_pos] & 0xFF;
                    plybck->buffer[plybck->front_pos+1] = full_audio[actual_pos] >> 8 & 0xFF;

                    if (plybck->front_pos >= plybck->size - plybck->BytesInSample)
                    {
                        plybck->front_pos = 0;
                    } else {
                        plybck->front_pos += plybck->BytesInSample;
                    }
                    stitch_pos += adv;
                    actual_pos = (int)stitch_pos;
                }
                memset(&full_audio[0], 0, AUDIO_SCAN_LEN*2);
            }
            if (sync_block_delay > 0)
            {
                sync_block_delay--;
            }

            luma_data[xCord][yCord] = (chan2 / luma_level) + 127;

            if (xCord > 13 && xCord < 56)
            {
                chroma_u_data[xCord-14][yCord] = (chan1 / chroma_level);
            }
            if (xCord > 55 && xCord < 98)
            {
                chroma_v_data[xCord-56][yCord] = chan1 / chroma_level;
            }

            if (xCord < scan_w)
            {
                xCord++;
            }

            if (audio_scan_pos < AUDIO_SCAN_LEN)
            {
                if (audio_scan_pos >= 0)
                {
                    c1_buffer.data[c1_buffer.pos++] = clip16(chan1 * sound_level);
                    c2_buffer.data[c2_buffer.pos++] = clip16(chan2 * sound_level);
                }
                audio_scan_pos++;
            }

            if (b->back_pos >= b->size - b->BytesInSample)
            {
                b->back_pos = 0;
            } else {
                b->back_pos += b->BytesInSample;
            }

            // adaptive sync threshold
            if (level_check1 > sync_1.max)
            {
                sync_1.max = level_check1;
            } else if (level_check1 < sync_1.min)
            {
                sync_1.min = level_check1;
            }

            if (level_check2 > sync_2.max)
            {
                sync_2.max = level_check2;
            } else if (level_check2 < sync_2.min)
            {
                sync_2.min = level_check2;
            }

            if (tick_count > adaptive_sync_interval)
            {
                set_sync_threshold();
                tick_count = 0;
            } else {
                tick_count++;
            }

        }
        else if (display_mode == CCI_DISPLAY)
        {
            if (chan1 > cci_sync_high && sync_block_delay == 0)
            {
                sync_block_delay = sync_delay;
                draw_cci_line(cci_tex, yCord);
                update_display = true;
                yCord = 0;
                xCord = 0;
            }
            if (chan1 < cci_sync_low && sync_block_delay == 0)
            {
                sync_block_delay = sync_delay;
                draw_cci_line(cci_tex, yCord);
                update_display = true;
                xCord = 0;
                if (yCord < 500)
                {
                    yCord++;
                } else {
                    yCord = 0;
                }
            }

            if (xCord >= 60 && xCord < 1560)
            {
                cci_l[xCord-60] = (int)((chan2 / 120) + 128);
                if (xCord >= 60 && xCord < 810)
                {
                    cci_u[xCord-60] = (int)chan1;
                } else if (xCord >= 810 && xCord < 1560)
                {
                    cci_v[xCord-810] = (int)chan1;
                }
            }
            if (xCord < 1600)
            {
                xCord++;
            }

            if (sync_block_delay > 0)
            {
                sync_block_delay--;
            }
        }
        else if (display_mode == CCV2_PLAYBACK)
        {
            if (chan2 > sync_2.sync_high && sync_block_delay == 0)
            {
                yCord = 0;
                interlace = 0;
                xCord = 0;
                update_display = true;
                sync_block_delay = sync_delay;
            }
            if (chan2 < sync_2.sync_low && sync_block_delay == 0)
            {
                yCord = 1;
                interlace = 1;
                xCord = 0;
                update_display = true;
                sync_block_delay = sync_delay;
            }
            if (chan1 > sync_1.sync_high && sync_block_delay == 0)
            {
                xCord = 0;
                sync_block_delay = sync_delay;
                if (yCord < 60)
                {
                    yCord += 4;
                } else if (yCord < 90)
                {
                    yCord += 2;
                }
            }
            if (chan1 < sync_1.sync_low && sync_block_delay == 0)
            {
                audio_scan_pos = 0 - audio_scan_offset;
                c1_buffer.pos = 0;
                c2_buffer.pos = 0;
                sync_block_delay = sync_delay;
                int ccv2_actual_pos = 0;
                double ccv2_stitch_pos = 0;
                double ccv2_adv = (double)CCV2_AUDIO_RATE / (double)sFreq;
                Sint16 ccv2_full_audio[CCV2_AUDIO_SCAN_LEN*2];
                stitch_audio(&ccv2_full_audio[0]);
                while (plybck->front_pos != plybck->back_pos && ccv2_actual_pos < 1360)
                {
                    plybck->buffer[plybck->front_pos] = ccv2_full_audio[ccv2_actual_pos] & 0xFF;
                    plybck->buffer[plybck->front_pos+1] = ccv2_full_audio[ccv2_actual_pos] >> 8 & 0xFF;

                    if (plybck->front_pos >= plybck->size - plybck->BytesInSample)
                    {
                        plybck->front_pos = 0;
                    } else {
                        plybck->front_pos += plybck->BytesInSample;
                    }
                    ccv2_stitch_pos += ccv2_adv;
                    ccv2_actual_pos = (int)ccv2_stitch_pos;
                }
                memset(&ccv2_full_audio[0], 0, CCV2_AUDIO_SCAN_LEN*2);
            }
            if (sync_block_delay > 0)
            {
                sync_block_delay--;
            }

            if (xCord >= 14 && xCord < 134)
            {
                luma_data[xCord-14][yCord] = (chan1 / luma_level) + 127;
                if (yCord < 60)
                {
                    luma_data[xCord-14][yCord+2] = (chan2 / luma_level) + 127;
                } else {
                    if (xCord >= 14 && xCord < 74)
                    {
                        ccv2_chroma_u.push_back((chan2 / chroma_level));
                    } else if (xCord >= 74 && xCord < 134)
                    {
                        ccv2_chroma_v.push_back((chan2 / chroma_level));
                    }
                }
            }

            if (xCord < 136)
            {
                xCord++;
            }

            if (audio_scan_pos < CCV2_AUDIO_SCAN_LEN)
            {
                if (audio_scan_pos >= 0)
                {
                    c1_buffer.data[c1_buffer.pos++] = clip16(chan1 * sound_level);
                    c2_buffer.data[c2_buffer.pos++] = clip16(chan2 * sound_level);
                }
                audio_scan_pos++;
            }

            if (b->back_pos >= b->size - b->BytesInSample)
            {
                b->back_pos = 0;
            } else {
                b->back_pos += b->BytesInSample;
            }

            // adaptive sync threshold
            if (level_check1 > sync_1.max)
            {
                sync_1.max = level_check1;
            } else if (level_check1 < sync_1.min)
            {
                sync_1.min = level_check1;
            }

            if (level_check2 > sync_2.max)
            {
                sync_2.max = level_check2;
            } else if (level_check2 < sync_2.min)
            {
                sync_2.min = level_check2;
            }

            if (tick_count > adaptive_sync_interval)
            {
                set_sync_threshold();
                tick_count = 0;
            } else {
                tick_count++;
            }
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////    Main
////////////////////////////////////////////////////////////////////////////////

int main ()
{
    int mode;
    chan1_level = 1.0;
    chan2_level = 1.0;
    printf("0 for CCV encoding \n1 for display\n2 for CCI encoding\n3 for CCV2 encoding\n");
	scanf("%d", &mode);
    if (mode != 0 && mode != 1 && mode != 2 && mode != 3)
    {
        printf("Invalid entry!\n");
        return 0;
    }

    if (mode == 2)
    {
        ccv_image make_image;
        return 0;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		return 1;
	}

    if (mode == 0)
    {
        Encoder encode;
        SDL_Quit();
        printf("Done!\n");
        return 0;
    }

    if (mode == 3)
    {
        ccv_2 ccv2_encode;
        SDL_Quit();
        return 0;
    }

    SDL_Window* window = NULL;
    SDL_Renderer* render = NULL;

    AudioBuffer audio_buffer;
    AudioBuffer sound_playback;

//// Open recording device
    SDL_AudioDeviceID recordingDeviceId = 0;
    SDL_AudioSpec desiredRecordingSpec;
    SDL_AudioSpec gReceivedRecordingSpec;
    SDL_zero(desiredRecordingSpec);
	desiredRecordingSpec.freq = sFreq;
	desiredRecordingSpec.format = AUDIO_S16;
	desiredRecordingSpec.channels = 2;
	desiredRecordingSpec.samples = SAMPLE_SIZE;
	desiredRecordingSpec.callback = audioRecordingCallback;
    desiredRecordingSpec.userdata = &audio_buffer;
	int gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);
	if(gRecordingDeviceCount < 1)
	{
		printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}
    printf("\n");
	int index;
	for(int i = 0; i < gRecordingDeviceCount; ++i)
	{
		//Get capture device name
		const char* deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);

		printf("%d - %s\n", i, deviceName);
	}
	printf("Choose signal input device\n");
	scanf("%d", &index);
	//Open recording device
	recordingDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_TRUE), SDL_TRUE, &desiredRecordingSpec, &gReceivedRecordingSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	// Device failed to open
	if(recordingDeviceId == 0)
	{
		//Report error
		printf("Failed to open recording device! SDL Error: %s", SDL_GetError() );
		return 1;
	}

    if (gReceivedRecordingSpec.format != AUDIO_S16 || gReceivedRecordingSpec.samples != SAMPLE_SIZE || gReceivedRecordingSpec.freq != sFreq)
    {
        printf("Unsuported audio format!\n");
        return 1;
    }

	//Calculate per sample bytes
	audio_buffer.BytesInSample = gReceivedRecordingSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedRecordingSpec.format) / 8);

	audio_buffer.size = SAMPLE_SIZE * 6;

    audio_buffer.front_pos = SAMPLE_SIZE * 3;
    audio_buffer.back_pos = 0;

	//Allocate and initialize byte buffer
	audio_buffer.buffer = new Uint8[audio_buffer.size];
	memset(audio_buffer.buffer, 0, audio_buffer.size);

///// Open playback device
    SDL_AudioDeviceID playbackDeviceId = 0;
    SDL_AudioSpec desiredPlaybackSpec;
    SDL_AudioSpec gReceivedPlaybackSpec;
    SDL_zero(desiredPlaybackSpec);
    desiredPlaybackSpec.freq = sFreq;
    desiredPlaybackSpec.format = AUDIO_S16;
    desiredPlaybackSpec.channels = 1;
    desiredPlaybackSpec.samples = SAMPLE_SIZE;
    desiredPlaybackSpec.callback = audioPlaybackCallback;
    desiredPlaybackSpec.userdata = &sound_playback;
    int gPlaybackDeviceCount = SDL_GetNumAudioDevices(SDL_FALSE);
    if(gPlaybackDeviceCount < 1)
	{
		printf( "Unable to get audio output device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}
    printf("\n");
    for(int i = 0; i < gPlaybackDeviceCount; ++i)
	{
		//Get capture device name
		const char* deviceName = SDL_GetAudioDeviceName(i, SDL_FALSE);

		printf("%d - %s\n", i, deviceName);
	}
	printf("Choose audio playback device\n");
	scanf("%d", &index);
    //Open playback device
	playbackDeviceId = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(index, SDL_FALSE), SDL_FALSE, &desiredPlaybackSpec, &gReceivedPlaybackSpec, SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	// Device failed to open
	if(playbackDeviceId == 0)
	{
		//Report error
		printf("Failed to open playback device! SDL Error: %s", SDL_GetError() );
		return 1;
	}

    if (gReceivedPlaybackSpec.format != AUDIO_S16 || gReceivedPlaybackSpec.samples != SAMPLE_SIZE
        || gReceivedPlaybackSpec.freq != sFreq || gReceivedPlaybackSpec.channels != 1)
    {
        printf("Unsuported audio format!\n");
        return 1;
    }

    //Calculate per sample bytes
	sound_playback.BytesInSample = gReceivedPlaybackSpec.channels * (SDL_AUDIO_BITSIZE(gReceivedPlaybackSpec.format) / 8);

	sound_playback.size = SAMPLE_SIZE * 6;

    sound_playback.front_pos = SAMPLE_SIZE * 3;
    sound_playback.back_pos = 0;

	//Allocate and initialize byte buffer
	sound_playback.buffer = new Uint8[sound_playback.size];
	memset(sound_playback.buffer, 0, sound_playback.size);


///// Window creation
    window = SDL_CreateWindow("Compact Cassette Video Player", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    screen_w, screen_h, SDL_WINDOW_SHOWN);
    if (window == NULL)
    {
        printf("Window creation failed!\n");
        return 2;
    }

    render = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (render == NULL)
    {
        printf("Failed to create renderer!\n");
        return 2;
    }

    SDL_Texture* display_tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, frame_w, frame_h);
    SDL_Texture* display_ccv2_tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 120, 90);
    SDL_Texture* display_image_tex = SDL_CreateTexture(render, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, 750, 500);
    SDL_Rect display_rec;
    display_rec.w = frame_w*SCALE;
    display_rec.h = frame_h*SCALE;
    display_rec.x = (screen_w / 2) - (display_rec.w / 2);
    display_rec.y = (screen_h / 2) - (display_rec.h / 2);

    SDL_Rect display_image_rec;
    display_image_rec.w = 750;
    display_image_rec.h = 500;
    display_image_rec.x = (screen_w / 2) - (display_image_rec.w / 2);
    display_image_rec.y = (screen_h / 2) - (display_image_rec.h / 2);

    clear_buffs();

    fmt = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));

	SDL_PauseAudioDevice( recordingDeviceId, SDL_FALSE );
    SDL_PauseAudioDevice( playbackDeviceId, SDL_FALSE );
    bool run = true;
    SDL_Event e;
    while (run)
    {
        while(SDL_PollEvent(&e))
        {
            switch (e.type)
            {
                case SDL_QUIT:
                    run = false;
                    break;
                case SDL_KEYDOWN:
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        run = false;
                    } else {
                        switch (e.key.keysym.sym)
                        {
                            case SDLK_r:
                                update_display = true;
                                break;
                            case SDLK_1:
                                display_mode = CCV_PLAYBACK;
                                update_display = true;
                                xCord = yCord = 0;
                                SDL_PauseAudioDevice( playbackDeviceId, SDL_FALSE );
                                break;
                            case SDLK_2:
                                display_mode = CCV2_PLAYBACK;
                                update_display = true;
                                xCord = yCord = 0;
                                SDL_PauseAudioDevice( playbackDeviceId, SDL_FALSE );
                                break;
                            case SDLK_3:
                                display_mode = CCI_DISPLAY;
                                update_display = true;
                                xCord = yCord = 0;
                                SDL_PauseAudioDevice( playbackDeviceId, SDL_TRUE );
                                break;
                            case SDLK_UP:
                                if (chan1_level < 15.0)
                                {
                                    chan1_level += 0.05;
                                }
                                break;
                            case SDLK_DOWN:
                                if (chan1_level > 0.1)
                                {
                                    chan1_level -= 0.05;
                                }
                                break;
                            case SDLK_RIGHT:
                                if (chan2_level < 15.0)
                                {
                                    chan2_level += 0.05;
                                }
                                break;
                            case SDLK_LEFT:
                                if (chan2_level > 0.1)
                                {
                                    chan2_level -= 0.05;
                                }
                                break;
                            case SDLK_i:
                                invert_signal = !invert_signal;
                                break;
                            case SDLK_c:
                                swap_channels = !swap_channels;
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                default:
                    break;
            }
        }

        scanner(&audio_buffer, &sound_playback, display_image_tex);

        if (update_display)
        {
            update_display = false;
            SDL_SetRenderDrawColor(render, 0, 0, 0, 0xFF); // Background color
            SDL_RenderClear(render);
            switch (display_mode)
            {
                case CCV_PLAYBACK:
                    draw(display_tex);
                    SDL_RenderCopy(render, display_tex, NULL, &display_rec);
                    break;
                case CCV2_PLAYBACK:
                    draw(display_ccv2_tex);
                    SDL_RenderCopy(render, display_ccv2_tex, NULL, &display_rec);
                    break;
                case CCI_DISPLAY:
                    SDL_RenderCopy(render, display_image_tex, NULL, &display_image_rec);
                    break;
                default:
                    break;
            }
            SDL_RenderPresent(render);
        }
    }

	delete[] audio_buffer.buffer;
	delete[] sound_playback.buffer;
    SDL_FreeFormat(fmt);
    SDL_CloseAudioDevice(recordingDeviceId);
    SDL_CloseAudioDevice(playbackDeviceId);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
