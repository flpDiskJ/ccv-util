#include "include/ccv_image.h"

ccv_image::ccv_image()
{
    char tmp[101];
    string temp;
    int transition_rate = 3;
    printf("Image Transition Rate (Seconds): ");
    scanf("%d", &transition_rate);
    printf("Type 'e' to break file entry\n");
    while (true)
    {
        printf("Image File(s): ");
        scanf("%100s", tmp);
        if (tmp[0] == 'e' && tmp[1] == '\0')
        {
            break;
        }
        temp = "image_input/";
        temp += tmp;
        files.push_back(temp);
    }

    printf("Processing...\n");

    for (int x = 0; x < files.size(); x++)
    {
        blank(sFreq * transition_rate);
        convert_image(x);
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

    sync_y();
    Sint32 chroma_u[frame_width], chroma_v[frame_width], luma[frame_width];
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
            l -= 128; // 128 <-> -127
            if (l > 130){l = 130;}
            if (l < -130){l = -130;}
            l *= (sync_high / 2) / 130;
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

void ccv_image::stitch(Sint32 *u, Sint32 *v, Sint32 *l)
{
    Sint16 val = 0;
    for (int i = 0; i < frame_width; i += 2)
    {
        val = (Sint16)((u[i] + u[i+1]) / 2);
        channel_1.push_back(val);
        channel_1.push_back(val);
    }
    for (int i = 0; i < frame_width; i += 2)
    {
        val = (Sint16)((v[i] + v[i+1]) / 2);
        channel_1.push_back(val);
        channel_1.push_back(val);
    }
    for (int i = 0; i < frame_width; i++)
    {
        val = (Sint16)l[i];
        channel_2.push_back(val);
        channel_2.push_back(val);
    }
    blank(40);
}

void ccv_image::blank(int sNum)
{
    for (int i = 0; i < sNum; i++)
    {
        channel_1.push_back(0);
        channel_2.push_back(0);
    }
}

void ccv_image::sync_x()
{
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_low);
        channel_2.push_back(sync_low);
    }
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_high);
        channel_2.push_back(sync_high);
    }
    blank(44);
}

void ccv_image::sync_y()
{
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_high);
        channel_2.push_back(sync_high);
    }
    for (int i = 0; i < 8; i++)
    {
        channel_1.push_back(sync_low);
        channel_2.push_back(sync_low);
    }
    blank(44);
}
