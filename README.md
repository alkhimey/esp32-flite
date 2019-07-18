# Flite on ESP32 Example

This project demonstrates speech synthesis on the ESP32. It performs the
synthesis locally using the [CMU Flite](http://www.festvox.org/flite/) library,
rather than offloading this task to cloud providers.

For this project, Flite 2.2 (commit hash e9880474) was ported to esp-idf 3.2.2
framework and is now a set of reusable components that can be found in the
"components" directory.

The `cmu_us_kal` voice is provided as an example. Other predefined voices that
come with Flite are too big to fit into the FLASH. New voices could be added as
separate components provided that they fit into FLASH.

## Running the Example
The example runs a simple http server that receives GET requests of text to be
synthesized. The program synthesizes the text and sends the PCM data over I2S.
On the I2S receiving side I used PCM5102 chip, but any other chip might work.
Additionally it could be possible to route I2S to an ESP32 internal 8 bit DAC.

First, configure using `make menuconfig`. You need to set your Wi-Fi SSID and
password as well as the pins to use for I2S. I tested with BCK = 26, WS = 25
and DATA = 22.

Since the produced WAV file is stored as an array of PCM values allocated on
the heap, enough heap space must be available. The space required depends on
the length of the synthesized text. Therefore using WROVER model of ESP32 that
have 4MB of PSARAM is advised. The PSRAM must be enabled in menuconfig. It is a
little hidden in the menus: Component config -> ESP32 Specific -> Support for
external, SPI connected RAM -> SPI RAM Config. Once enabled, it will bre added
to the heap allocation pool.

To send the text for ESP32 to synthesize, one need to send an http GET request
of `/say` path with a query parameter `s`. This can be done with a web browse.
Browse to `http://<ip of esp device>/say?s=This is an example text`. The query
string is limited to approximately 256 characters, but this is an artificial
limitation of the example program and the Flite library can synthesize much
longer texts at once.

The synthesized data is streamed in chunks, thus playback can begin before
Flite finished processing all of the text. This reduces delay for longer texts
and gives a real time feel. This is one of the advantages of using Flite rather
than using cloud services and downloading the synthesized data via Wi-Fi.

## Adding to Your Project

1. Copy the components into your project. 
2. Make sure your app partiton has at least 2MB.

        factory,  app,  factory, 0x10000, 0x2F0000,

3. Configure with `make menuconfig`.

4. Then use the following code:

        cst_voice *register_cmu_us_kal(const char *voxdir);
        int i2s_stream_chunk(const cst_wave *w, int start, int size, 
                        int last, cst_audio_streaming_info *asi)
        {
            // write here code that processes the wav chunk. For example send it to
            // I2S, drive a DAC or send it via Wi-Fi/Bluetooth/Serial to another
            // device.
        }
        ...
        
        /* Initialization code */
        flite_init();        
        cst_voice *v = register_cmu_us_kal(NULL);

        cst_audio_streaming_info *asi = 
            cst_alloc(struct cst_audio_streaming_info_struct,1);

        asi->min_buffsize = 256;
        asi->asc = i2s_stream_chunk;
        asi->userdata = NULL;

        feat_set(v->features,"streaming_info",audio_streaming_info_val(asi));

        /* Synthesis Code */
        cst_wave * wav = flite_text_to_wave("Replace with your text",v);
        delete_wave(wav);

## Use Case Ideas

* Talking clock and calendar
* Talking weather station
* News reader
* Mail or Twitter reader
* Chat bot
* Personal assistant
* Talking toys
* Educational games

## Projects That Use Flite

If you have used Flite in your project, open a pull request with a link to the
project and I will add it here.