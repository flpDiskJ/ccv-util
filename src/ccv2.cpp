#include "include/ccv2.h"

ccv_2::ccv_2()
{
    string temp;
    int frame_pos = 0, total_frames = 0;
    printf("Start frame (without z fill): ");
    scanf("%d", &frame_pos);
    printf("End frame (without z fill): ");
    scanf("%d", &total_frames);
    printf("Amount of z fill: ");
    scanf("%d", &zFill);
    printf("%d - %d\n", frame_pos, total_frames);
    printf("Processing...\n");

    load_audio();

    blank(sFreq * 3);
    for (int x = frame_pos; x <= total_frames; x++)
    {
        temp = "input/" + fill(x) + ".png";
        convert_image(temp);
    }
    blank(sFreq * 3);

    if (channel_1.size() != channel_2.size())
    {
        printf("size missmatch\n");
        printf("%d %d\n", channel_1.size(), channel_2.size());
        return;
    }

    make_wav();

    printf("Done!\n");
}

ccv_2::~ccv_2()
{
    if (audio_length > 0)
    {
        free(audio);
    }
}

void ccv_2::convert_image(string filename)
{
    unsigned char r, g, b; // RGB Pixel values
    Sint16 l, u, v; // YUV Pixel values
    int width, height, channels;
    unsigned char *img = stbi_load(filename.c_str(), &width, &height, &channels, 3);
    if (img == NULL)
    {
        printf("Couldn't load image: %s\n", filename.c_str());
        return;
    } else if (width != frame_width || height != frame_height || channels != 3)
    {
        printf("Incorrect specs! Image: %s\n", filename.c_str());
        printf("Required: 120x90 8bit RGB PNG\n");
        free(img);
        return;
    }

    // prepare data
    for (int y = y_start; y < frame_height; y += 2)
    {
        for (int x = 0; x < frame_width; x++)
        {
            r = img[3 * ((y*frame_width) + x)    ];
            g = img[3 * ((y*frame_width) + x) + 1];
            b = img[3 * ((y*frame_width) + x) + 2];
            l = 0.299*r + 0.587*g + 0.114*b;
            u = -0.147*r - 0.289*g + 0.436*b;
            v = 0.615*r - 0.515*g - 0.100*b;
            if (l > 255){l = 255;}
            if (l < 0){l = 0;}
            l -= 128;
            if (u > 128){u = 128;}
            if (u < -127){u = -127;}
            if (v > 128){v = 128;}
            if (v < -127){v = -127;}
            l = l * signal_amp;
            luma.push_back(l);
            chroma_u.push_back(u);
            chroma_v.push_back(v);
        }
    }

    chroma_downscale();

    unsigned int luma_pos = 0, chroma_u_pos = 0, chroma_v_pos = 0;

    // generate signal
    sync_y(y_offset);
    for (int l = 0; l < 30; l++)
    {
        if (l != 0)
        {
            sync_x();
        }
        for (int x = 0; x < frame_width; x++)
        {
            channel_1.push_back(luma[luma_pos++]);
        }
        for (int x = 0; x < frame_width; x++)
        {
            if (l < 15)
            {
                channel_2.push_back(luma[luma_pos++]);
            } else {
                if (x < 60)
                {
                    channel_2.push_back(chroma_u_downscale[chroma_u_pos++]);
                } else {
                    channel_2.push_back(chroma_v_downscale[chroma_v_pos++]);
                }
            }
        }
        blank(2);
    }

    // add audio to signal
    sync_audio();
    for (int i = 0; i < 680; i++)
    {
        if (audio_pos < audio_length)
        {
            channel_1.push_back(audio[audio_pos++]);
        } else {
            channel_1.push_back(0);
        }
    }
    for (int i = 0; i < 680; i++)
    {
        if (audio_pos < audio_length)
        {
            channel_2.push_back(audio[audio_pos++]);
        } else {
            channel_2.push_back(0);
        }
    }
    blank(10);

    // clear vectors
    luma.clear();
    chroma_u_downscale.clear();
    chroma_v_downscale.clear();

    free(img);
}

void ccv_2::make_wav()
{
    AudioFile<int> audioFile;
    // Setup audioFile specs
    audioFile.setAudioBufferSize (2, channel_1.size());
    audioFile.setBitDepth (16);
    audioFile.setSampleRate (sFreq);
    for (int x = 0; x < channel_1.size(); x++)
    {
        audioFile.samples[0][x] = channel_1[x] * output_signal_amp;
        audioFile.samples[1][x] = channel_2[x] * output_signal_amp;
    }
    audioFile.save ("output/CCV2_signal.wav", AudioFileFormat::Wave);
}

bool ccv_2::load_audio()
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
        double adv = (double)CCV2_AUDIO_RATE / (double)inputSpec.freq;
        if (inputSpec.channels == 1)
        {
            audio_length = (buffer_length * adv) + 100;
            audio = (Sint16*)malloc(sizeof(Sint16)*audio_length);
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
            audio = (Sint16*)malloc(sizeof(Sint16)*audio_length);
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
    Sint16 peak = 0;
    for (int x = 0; x < audio_length; x++)
    {
        if (audio[x] > peak)
        {peak = audio[x];}
    }
    // convert range
    double amp = (double)peak / (double)audio_peak;
    for (int x = 0; x < audio_length; x++)
    {
        audio[x] = (Sint16)((double)audio[x] / (double)amp);
    }
    return true;
}

void ccv_2::chroma_downscale()
{
    // 5400 = size of full chroma data (45lines * 120pixels)
    // 1800 = total size of chroma data (15lines * 120pixels)
    Sint16 val = 0;
    for (int i = 0; i < 5400; i += 6)
    {
        val = chroma_u[i];
        val += chroma_u[i+1];
        val += chroma_u[i+2];
        val += chroma_u[i+3];
        val += chroma_u[i+4];
        val += chroma_u[i+5];
        val /= 6;
        val *= chroma_signal_amp;
        chroma_u_downscale.push_back(val);

        val = chroma_v[i];
        val += chroma_v[i+1];
        val += chroma_v[i+2];
        val += chroma_v[i+3];
        val += chroma_v[i+4];
        val += chroma_v[i+5];
        val /= 6;
        val *= chroma_signal_amp;
        chroma_v_downscale.push_back(val);
    }
    chroma_u.clear();
    chroma_v.clear();
}

void ccv_2::blank(int sNum)
{
    for (int i = 0; i < sNum; i++)
    {
        channel_1.push_back(0);
        channel_2.push_back(0);
    }
}

void ccv_2::sync_x()
{
    for (int i = 0; i < 6; i++)
    {
        channel_1.push_back(SYNC_HI);
        channel_2.push_back(0);
    }
    for (int i = 0; i < 4; i++)
    {
        channel_1.push_back(SYNC_LO);
        channel_2.push_back(0);
    }
    blank(4);
}

void ccv_2::sync_audio()
{
    for (int i = 0; i < 6; i++)
    {
        channel_1.push_back(SYNC_LO);
        channel_2.push_back(0);
    }
    for (int i = 0; i < 4; i++)
    {
        channel_1.push_back(SYNC_HI);
        channel_2.push_back(0);
    }
    blank(20);
}

void ccv_2::sync_y(bool offset)
{
    if (!offset)
    {
        y_start = 0;
        for (int i = 0; i < 6; i++)
        {
            channel_2.push_back(SYNC_HI);
            channel_1.push_back(0);
        }
        for (int i = 0; i < 4; i++)
        {
            channel_2.push_back(SYNC_LO);
            channel_1.push_back(0);
        }
    } else {
        y_start = 1;
        for (int i = 0; i < 6; i++)
        {
            channel_2.push_back(SYNC_LO);
            channel_1.push_back(0);
        }
        for (int i = 0; i < 4; i++)
        {
            channel_2.push_back(SYNC_HI);
            channel_1.push_back(0);
        }
    }
    blank(4);
}

string ccv_2::fill(int num)
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
