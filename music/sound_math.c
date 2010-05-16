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
    double           s_cross,c_cross;
    volatile double  amplitude_in;
  } wave_form;

typedef struct {
    wave_form tones[MAX_TONES];
    int       length;
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

    for(unsigned int i=0; i < table->length; i++) {
        table->tones[i].s_cross = 0.0;
        table->tones[i].c_cross = 0.0;
    }
 
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
        for(unsigned int i=0; i < table->length; i++) {
            table->tones[i].phase += table->tones[i].phase_increment;
            if( table->tones[i].phase > Pi ) table->tones[i].phase -= 2*Pi;
            float s_amp = (float) sin( table->tones[i].phase );
            float c_amp = (float) cos( table->tones[i].phase );
            left_total  += s_amp*table->tones[i].left_amplitude;
            right_total += s_amp*table->tones[i].right_amplitude;
            table->tones[i].s_cross += s_amp*data_in;
            table->tones[i].c_cross += c_amp*data_in;
        }
        *out++ = left_total;
        *out++ = right_total;
    }

    for(unsigned int i=0; i < table->length; i++) {
        table->tones[i].amplitude_in = 
            0.9*table->tones[i].amplitude_in +
            0.1*(
              table->tones[i].s_cross*table->tones[i].s_cross+
              table->tones[i].c_cross*table->tones[i].c_cross
            );
    }
 
    return paContinue;
}

void add_tone(tone_table* table,float l_amp,float r_amp,float freq) {
    int i = table->length;
    /* TODO: Range check! */
    table->length += 1;
    table->tones[i].left_amplitude  = l_amp;
    table->tones[i].right_amplitude = r_amp;
    table->tones[i].amplitude_in    = 0.0;
    table->tones[i].frequency       = freq;
    table->tones[i].phase           = 0.0;
    table->tones[i].phase_increment = freq;
       /* 
       TODO: This is WRONG; the only reason it works is I'm passing the wrong values in as well 
       TODO: phase_increment should be a constant times the data->xxx.frequency
       TODO: The constant should be in the user data, and represent
         2*Pi/Samples_per_second or something like that
       */
}


#define Stereo  2
#define Mono    1
#define Seconds 1000
int main(void)
{
    int const               Frames_per_buffer = 64;
    float const             Sample_rate = 44100.0;

    tone_table table;
    table.length = 0;
    add_tone(&table,1.0,1.0,0.01);
    add_tone(&table,1.0,1.0,0.06);
    add_tone(&table,0.0,0.0,0.09);
    add_tone(&table,0.0,0.0,0.03);

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

    for(int i=0;i<table.length;i++) {
        printf("%5.3f was at %10.5f\n",table.tones[i].frequency,table.tones[i].amplitude_in*100);
    }

    Pa_Terminate();
    return 0;
}

