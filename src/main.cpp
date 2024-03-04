// Code by Jake Aigner December 2023
// Functional: March, 2024

#define STB_IMAGE_IMPLEMENTATION
#include "include/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "include/stb_image_write.h"
#include "include/AudioFile.h"
#include "include/global.h"

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
    } // load_audio()

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
        for (int s = 0; s < 3; s++, chan1_pos++)
        {
            channel1[chan1_pos] = SYNC_LO;
        }
        blank(1, 2);
    }

    void sync_y() // 10s
    {
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_HI;
        }
        for (int s = 0; s < 3; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_LO;
        }
        blank(2, 2);
    }

    void sync_y_offset() // 10s
    {
        for (int s = 0; s < 5; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_LO;
        }
        for (int s = 0; s < 3; s++, chan2_pos++)
        {
            channel2[chan2_pos] = SYNC_HI;
        }
        blank(2, 2);
    }

    void write_wav()
    {
        AudioFile<int> audioFile;
        // Setup audioFile specs
        audioFile.setAudioBufferSize (2, signal_length);
        audioFile.setBitDepth (16);
        audioFile.setSampleRate (48000);
        for (int x = 0; x < signal_length; x++)
        {
            audioFile.samples[0][x] = channel1[x] * output_signal_amp;
            audioFile.samples[1][x] = channel2[x] * output_signal_amp;
        }
        audioFile.save ("output/signal.wav", AudioFileFormat::Wave);
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
                    u = (u * chroma_amp);
                    v = (v * chroma_amp);
                    l = (l * luma_amp) - (((luma_amp*255)-255)/2);
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
            for (int a = 0; a < 800; a++, audio_pos++, chan1_pos++)
            {
                channel1[chan1_pos] = audio[audio_pos];
            }
            for (int a = 0; a < 800; a++, audio_pos++, chan2_pos++)
            {
                channel2[chan2_pos] = audio[audio_pos];
            }
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
    memset(u_line, 0, scan_w);
    memset(v_line, 0, scan_w);
    int r, g, b, l, u, v;
    void *pixels;
    int pitch;
    SDL_LockTexture(tex, NULL, &pixels, &pitch);
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
    }
    SDL_UnlockTexture(tex);
}

void scanner(AudioBuffer *b)
{
    while (b->back_pos != b->front_pos)
    {
        if (swap_endianess)
        {
            chan2 = ((b->buffer[b->back_pos] & 0xFF) << 8) | (b->buffer[b->back_pos+1] & 0xFF);
            chan1 = ((b->buffer[b->back_pos+2] & 0xFF) << 8) | (b->buffer[b->back_pos+3] & 0xFF);
        } else {
            chan1 = ((b->buffer[b->back_pos+1] & 0xFF) << 8) | (b->buffer[b->back_pos] & 0xFF);
            chan2 = ((b->buffer[b->back_pos+3] & 0xFF) << 8) | (b->buffer[b->back_pos+2] & 0xFF);
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

        if (chan2 > hi_sync_threshold && sync_block_delay == 0)
        {
            yCord = 0;
            interlace = 0;
            xCord = 0;
            update_display = true;
            sync_block_delay = sync_delay;
        }
        if (chan2 < lo_sync_threshold && sync_block_delay == 0)
        {
            yCord = 1;
            interlace = 1;
            xCord = 0;
            update_display = true;
            sync_block_delay = sync_delay;
        }
        if (chan1 > hi_sync_threshold && sync_block_delay == 0)
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

        if (b->back_pos >= b->size - b->BytesInSample)
        {
            b->back_pos = 0;
        } else {
            b->back_pos += b->BytesInSample;
        }

    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////    Main
////////////////////////////////////////////////////////////////////////////////

int main ()
{
    int mode;
    printf("0 for Encoding | 1 for Player\n");
	scanf("%d", &mode);
    if (mode != 0 && mode != 1)
    {
        printf("Invalid entry!\n");
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
        return 0;
    }

    AudioBuffer audio_buffer;

    SDL_Window* window = NULL;
    SDL_Renderer* render = NULL;
    SDL_AudioDeviceID recordingDeviceId = 0;
    SDL_AudioSpec desiredRecordingSpec;
    SDL_zero(desiredRecordingSpec);
	desiredRecordingSpec.freq = sFreq;
	desiredRecordingSpec.format = AUDIO_S16;
	desiredRecordingSpec.channels = 2;
	desiredRecordingSpec.samples = SAMPLE_SIZE;
	desiredRecordingSpec.callback = audioRecordingCallback;
    desiredRecordingSpec.userdata = &audio_buffer;
	gRecordingDeviceCount = SDL_GetNumAudioDevices(SDL_TRUE);
	if(gRecordingDeviceCount < 1)
	{
		printf( "Unable to get audio capture device! SDL Error: %s\n", SDL_GetError() );
		return 0;
	}
	int index;
	for(int i = 0; i < gRecordingDeviceCount; ++i)
	{
		//Get capture device name
		const char* deviceName = SDL_GetAudioDeviceName(i, SDL_TRUE);

		printf("%d - %s\n", i, deviceName);
	}
	printf("Choose audio input device\n");
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

	audio_buffer.size = (SAMPLE_SIZE * audio_buffer.BytesInSample) * 4;

	//Allocate and initialize byte buffer
	audio_buffer.buffer = new Uint8[audio_buffer.size];
	memset(audio_buffer.buffer, 0, audio_buffer.size);

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
    SDL_Rect display_rec;
    display_rec.w = frame_w*SCALE;
    display_rec.h = frame_h*SCALE;
    display_rec.x = (screen_w / 2) - (display_rec.w / 2);
    display_rec.y = (screen_h / 2) - (display_rec.h / 2);

    hi_sync_threshold = (double)(pow(2, 16) / 2) / sync_detect_sensitivity;
    lo_sync_threshold = 0 - hi_sync_threshold;

    clear_buffs();

    fmt = SDL_AllocFormat(SDL_GetWindowPixelFormat(window));

    // begin recording
	//Go back to beginning of buffer
	audio_buffer.front_pos = 0;
    audio_buffer.back_pos = 0;
	SDL_PauseAudioDevice( recordingDeviceId, SDL_FALSE );
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

        scanner(&audio_buffer);

        if (update_display)
        {
            update_display = false;
            draw(display_tex);
            SDL_SetRenderDrawColor(render, 0, 0, 0, 0xFF); // Background color
            SDL_RenderClear(render);
            SDL_RenderCopy(render, display_tex, NULL, &display_rec);
            SDL_RenderPresent(render);
        }
    }

	if( audio_buffer.buffer != NULL )
	{
		delete[] audio_buffer.buffer;
		audio_buffer.buffer = NULL;
	}

    SDL_FreeFormat(fmt);
    SDL_CloseAudioDevice(recordingDeviceId);
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
