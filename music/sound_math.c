#include "stdio.h"
#include "portaudio.h"
#include <math.h>


#define Pi        (3.14159265)
#define MAX_TONES (10)

typedef struct {
    double           phase;
    double           frequency;
    double           phase_increment;
    double           left_amplitude,right_amplitude;
    volatile double  amplitude_in;
  } wave_form;

typedef struct {
    wave_form tones[MAX_TONES];
    int length;
} tone_table;

static int my_callback(
    const void*                     inputBuffer,
    void*                           outputBuffer,
    unsigned long                   framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags           statusFlags,
    void*                           userData
    )
{
    const float* in  = (const float *) inputBuffer;
    float*       out = (float *)       outputBuffer;
    float        data_in;

    tone_table* table = (tone_table*)userData;

    /* Read input buffer, process data, and fill output buffer. */
    for(unsigned int frame=0; frame<framesPerBuffer; frame++ ) {
        data_in  = *in++;

        /*
        TODO: It should compute the cross sections of the data_in with each wave (sin & cos) and then
          update their amplitude_in value after the loop (e.g., in a final loop).  This may require
          temporary storage somewhere?
        */

        float  left_total  = 0.0;
        float  right_total = 0.0;
        float  amp = 0.0;
        for(unsigned int i=0; i < table->length; i++) {
            table->tones[i].phase += table->tones[i].phase_increment;
            if( table->tones[i].phase > Pi ) table->tones[i].phase -= 2*Pi;
            amp = (float) sin( table->tones[i].phase );
            left_total  += amp*table->tones[i].left_amplitude;
            right_total += amp*table->tones[i].right_amplitude;
        }
        *out++ = left_total;
        *out++ = right_total;
    }

    return paContinue;
}

#define Stereo  2
#define Mono    1
#define Seconds 1000
int main(void)
{
    int const               Frames_per_buffer = 64;
    float const             Sample_rate = 44100.0;

    tone_table table;

    table.tones[0].left_amplitude  = 1.0;
    table.tones[1].right_amplitude = 1.0;
    table.tones[0].left_amplitude  = 1.0;
    table.tones[1].right_amplitude = 1.0;
    table.tones[0].phase      = 0.0;
    table.tones[1].phase     = 0.0;
    table.tones[0].phase_increment  = 0.01;
    table.tones[1].phase_increment = 0.06;

    table.length = 2; // It's a start.

       /*
       TODO: phase_increment should be a constant times the data->xxx.frequency
       TODO: The constant should be in the user data, and represent
         2*Pi/Samples_per_second or something like that
       */

    PaStream *stream;

    Pa_Initialize();

    Pa_OpenDefaultStream(
        &stream, Mono/*in*/, Stereo/*out*/, paFloat32, Sample_rate, Frames_per_buffer,
        my_callback, &table
        );
    Pa_StartStream( stream );
    Pa_Sleep( 10*Seconds );
    Pa_StopStream( stream );
    Pa_CloseStream( stream );


    Pa_Terminate();
    return 0;
}

