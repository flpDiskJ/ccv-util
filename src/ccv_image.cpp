#include "include/ccv_image.h"

ccv_image::ccv_image()
{
    string temp;
    int transition_rate = 3;
    int photos = 0;
    printf("Image Transition Rate (Seconds): ");
    scanf("%d", &transition_rate);
    printf("Number of images to encode: ");
    scanf("%d", &photos);
    int image = 0;
    while (image <= photos)
    {
        temp = "image_input/";
        temp += to_string(image);
        temp += ".png";
        files.push_back(temp);
        image++;
    }

    printf("Processing...\n");

    for (int x = 0; x < files.size(); x++)
    {
        blank(sFreq * transition_rate);
        convert_image(x);
    }
    blank(sFreq * 3);

    make_wav();

    printf("Done!\n");
}

ccv_image::~ccv_image()
{

}

void ccv_image::convert_image(int i)
{
    unsigned char r, g, b; // RGB Pixel values
    Sint32 l, u, v; // YUV Pixel values
    int width, height, channels;
    unsigned char *img = stbi_load(files[i].c_str(), &width, &height, &channels, 3);
    if (img == NULL)
    {
        printf("Couldn't load image: %s\n", files[i].c_str());
        return;
    } else if (width != frame_width || height != frame_height || channels != 3)
    {
        printf("Incorrect specs! Image: %s\n", files[i].c_str());
        printf("Required: 750x500 8bit RGB PNG\n");
        free(img);
        return;
    }

    for (int i = 0; i < 4; i++) // garbage sync pulses used to set the sync detect during display
    {
        sync_y();
        blank(750);
        sync_x();
        blank(750);
    }

    sync_y();
    Sint16 chroma_u[frame_width], chroma_v[frame_width], luma[frame_width];
    for (int y = 0; y < frame_height; y++)
    {
        if (y > 0)
        {
            sync_x();
        }
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
            if (u > 130){u = 130;}
            if (u < -130){u = -130;}
            if (v > 130){v = 130;}
            if (v < -130){v = -130;}
            chroma_u[x] = u * ((sync_high / 5) / 130);
            chroma_v[x] = v * ((sync_high / 5) / 130);
            luma[x] = l;
        }
        stitch(chroma_u, chroma_v, luma);
    }

    free(img);
}

void ccv_image::make_wav()
{
    AudioFile<int> audioFile;
    // Setup audioFile specs
    audioFile.setAudioBufferSize (2, channel_1.size());
    audioFile.setBitDepth (16);
    audioFile.setSampleRate (sFreq);
    for (int x = 0; x < channel_1.size(); x++)
    {
        audioFile.samples[0][x] = channel_1[x] * signal_multiplier;
        audioFile.samples[1][x] = channel_2[x] * signal_multiplier;
    }
    audioFile.save ("output/CCI_signal.wav", AudioFileFormat::Wave);
}

void ccv_image::push_modulation(Sint16 val)
{
    val = val * modulation_freq[mod_pos];
    channel_2.push_back(val);
    if (mod_pos < 4)
    {
        mod_pos++;
    } else {
        mod_pos = 0;
    }
}

void ccv_image::stitch(Sint16 *u, Sint16 *v, Sint16 *l)
{
    Sint16 val = 0;
    for (int i = 0; i < frame_width; i += 2)
    {
        val = (u[i] + u[i+1]) / 2;
        channel_1.push_back(val);
        channel_1.push_back(val);
    }
    for (int i = 0; i < frame_width; i += 2)
    {
        val = (v[i] + v[i+1]) / 2;
        channel_1.push_back(val);
        channel_1.push_back(val);
    }
    for (int i = 0; i < frame_width; i++)
    {
        push_modulation(l[i]);
        push_modulation(l[i]);
    }
    blank(40);
}

void ccv_image::blank(int sNum)
{
    for (int i = 0; i < sNum; i++)
    {
        channel_1.push_back(0);
        push_modulation(0);
    }
}

void ccv_image::sync_x()
{
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_low);
        push_modulation(0);
    }
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_high);
        push_modulation(0);
    }
    blank(44);
}

void ccv_image::sync_y()
{
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_high);
        push_modulation(0);
    }
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_low);
        push_modulation(0);
    }
    blank(44);
}
