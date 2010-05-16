#include "stdio.h"
#include "portaudio.h"
#include <math.h>


#define Pi        (3.14159265)
#define Max_tones (10)

typedef struct {
    double           phase;
    double           frequency;
    double           phase_increment;
    double           left_amplitude,right_amplitude;
    volatile double  amplitude_in;
  } wave_form;

#define LEFT 0
#define RIGHT 1
#define A_hard_coded_2_because_we_lost_our_struct 2

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

    wave_form* freqs = (wave_form*)userData;

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
        for(unsigned int i=0; i < A_hard_coded_2_because_we_lost_our_struct; i++) {
            freqs[i].phase += freqs[i].phase_increment;
            if( freqs[i].phase > Pi ) freqs[i].phase -= 2*Pi;
            amp = (float) sin( freqs[i].phase );
            left_total  += amp*freqs[i].left_amplitude;
            right_total += amp*freqs[i].right_amplitude;
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

    wave_form frequencies[2];

    frequencies[LEFT].amplitude  = 1.0;
    frequencies[RIGHT].amplitude = 1.0;
    frequencies[LEFT].phase      = 0.0;
    frequencies[RIGHT].phase     = 0.0;
    frequencies[LEFT].phase_increment  = 0.01;
    frequencies[RIGHT].phase_increment = 0.06;

       /*
       TODO: phase_increment should be a constant times the data->xxx.frequency
       TODO: The constant should be in the user data, and represent
         2*Pi/Samples_per_second or something like that
       */


    PaStream *stream;

    Pa_Initialize();

    Pa_OpenDefaultStream(
        &stream, Mono/*in*/, Stereo/*out*/, paFloat32, Sample_rate, Frames_per_buffer,
        my_callback, &frequencies
        );
    Pa_StartStream( stream );
    Pa_Sleep( 10*Seconds );
    Pa_StopStream( stream );
    Pa_CloseStream( stream );


    Pa_Terminate();
    return 0;
}

