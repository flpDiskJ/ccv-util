Code by Jake Aigner 2024

Note: CCV2 audio playback is not functioning properly atm...

Info:
    Color space is converted from RGB to YUV.
    This allows color data to be reduced without degrading the overall quality-
    -which allows for more efficient use of the low bandwidth.

    CCV1 and CCI halve the color resolution
    CCV2 cuts the color down to 1/6th which allows for 30 more vertical scan-lines.

    CCI uses amplitude modulation on the luminance data to deter streaking.

    IMPORTANT!:
        you must create the following dirs inside of your build directory..
        "output"
        "input"
        "image_input"

    Image sequence (001.png 002.png 003.png...) and audio (audio.wav) for CCV and CCV2-
    -go inside the "input" dir.

    Images for CCI go inside the "image_input" dir (1.png 2.png 3.png...)

    Once encoded the signal will be generated as a wav file in the output dir.
    This signal can then be recorded to tape.

    The signals are designed to work with cassettes but should also work with reel 2 reel or anything-
    -else that meets the following requirements:
        At least 14kHz frequency response (compact cassettes achieve 15kHz on a decent deck)
        2 channels (total separation)
        Low amount of wow/flutter

CCV1 vs CCV2:
    CCV1 has better color and higher frame-rate.
    CCV2 has better resolution and better audio.

CCV1 (compact cassette video):
    Input Specs:
      PNG Image Sequence (8bit RGB)
      85 x 64 pixels (4:3)
      12 FPS
      Audio: Signed 16bit WAV, 1 or 2 channel, 48 or 44.1 kHz
    Actual Specs:
      25ish X 64 (note: using a reel2reel or something with higher bandwidth may increase X resolution)
      12i Refresh
      Audio: 12.2kHz mono

CCV2 (compact cassette video v2):
  Input Specs:
    PNG Image Sequence (8bit RGB)
    120 x 90 pixels (4:3)
    10 FPS
    Audio: Signed 16bit WAV, 1 or 2 channel, 48 or 44.1 kHz
  Actual Specs:
    36ish X 90 (note: using a reel2reel or something with higher bandwidth may increase X resolution)
    10i Refresh
    Audio: 13.6kHz mono

CCI (compact cassette image):
    Input Specs:
        750w x 500h
        8bit RGB
        PNG(s)
    Actual Specs:
        100ish x 500 (note: higher bandwidth won't increase quality much since it-
            -is limited by signal modulation)



Player Info:

  'ESC' to quit

  'i' key to toggle signal inversion

  'c' key to swap input channels

  'e' key to swap byte endian

  'r' to refresh screen

  Arrows adjust chan 1 and 2 level

  Modes:
      '1' = CCV1
      '2' = CCV2
      '3' = CCI
