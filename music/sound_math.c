#include "stdio.h"
#include "portaudio.h"
#include <math.h>


#define Pi        (3.14159265)
#define MAX_TONES (10)
#define Stereo  2
#define Mono    1
#define Seconds 1000
#define Frames_per_buffer 64
#define Sample_rate 44100.0

typedef struct {
    double           phase;
    double           frequency;
    double           phase_increment;
    double           left_amplitude,right_amplitude;
    double           s_cross,c_cross;
    volatile double  amplitude_in;
  } wave_form;

typedef struct {
    int       tones;
    wave_form tone[MAX_TONES];
    long      samples;
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

    for(unsigned int frame=0; frame<framesPerBuffer; frame++ ) {
        table->samples += 1;
        data_in  = *in++;
        float  left_total  = 0.0;
        float  right_total = 0.0;
        for(unsigned int i=0; i < table->tones; i++) {
            table->tone[i].phase += table->tone[i].phase_increment;
            if( table->tone[i].phase > Pi ) table->tone[i].phase -= 2*Pi;
            float s_amp = (float) sin( table->tone[i].phase );
            float c_amp = (float) cos( table->tone[i].phase );
            left_total  += s_amp*table->tone[i].left_amplitude;
            right_total += s_amp*table->tone[i].right_amplitude;
            table->tone[i].s_cross += s_amp*data_in;
            table->tone[i].c_cross += c_amp*data_in;
        }
        *out++ = left_total;
        *out++ = right_total;
    }

    for(unsigned int i=0; i < table->tones; i++) {
        wave_form* t = &(table->tone[i]);
        t->amplitude_in = (t->s_cross*t->s_cross+t->c_cross*t->c_cross)/table->samples;
    }
 
    return paContinue;
}

void add_tone(tone_table* table,float l_amp,float r_amp,float freq) {
    int i = table->tones;
    /* TODO: Range check! */
    table->tones += 1;
    table->tone[i].left_amplitude  = l_amp;
    table->tone[i].right_amplitude = r_amp;
    table->tone[i].amplitude_in    = 0.0;
    table->tone[i].frequency       = freq;
    table->tone[i].phase           = 0.0;
    table->tone[i].s_cross         = 0.0;
    table->tone[i].c_cross         = 0.0;
    table->tone[i].phase_increment = freq*2*Pi/Sample_rate;
}


int main(void)
{
    float half_step = 1.0594630943593;
    float full_step = half_step*half_step;
    float A_1 = 440.0;
    float B_1 = A_1*full_step;
    float C_2 = B_1*half_step;
    float D_2 = C_2*full_step;
    float E_2 = D_2*full_step;
    float F_2 = E_2*half_step;
    tone_table table;
    table.tones = 0;
    table.samples = 0;
    add_tone(&table,0.0,0.0,A_1);
    add_tone(&table,0.0,0.0,B_1);
    add_tone(&table,1.0,0.0,C_2);
    add_tone(&table,0.0,0.0,D_2);
    add_tone(&table,0.0,0.5,E_2);
    add_tone(&table,0.0,0.0,F_2);

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

    for(int i=0;i<table.tones;i++) {
        printf("%8.3f (%5.3f) was at %10.5f\n",
           table.tone[i].frequency,
           table.tone[i].phase_increment,
           table.tone[i].amplitude_in/100.0
           );
    }

    Pa_Terminate();
    return 0;
}

