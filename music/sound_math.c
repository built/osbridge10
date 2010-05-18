#include "stdio.h"
#include "portaudio.h"
#include <math.h>
#include <unistd.h>
#include <string.h>

#define Pi        (3.14159265)
#define MAX_TONES (20)
#define Stereo  2
#define Mono    1
#define Second  *1000
#define Seconds Second
#define Millisecond  *1
#define Milliseconds Millisecond
/* 0 Frames per buffer means use the optimal # */
#define Frames_per_buffer 0
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

#define Half_step 1.0594630943593
#define Full_step (Half_step*Half_step)
#define A_1       220.0
#define B_1       (A_1*Full_step)
#define C_2       (B_1*Half_step)
#define D_2       (C_2*Full_step)
#define E_2       (D_2*Full_step)
#define F_2       (E_2*Half_step)
#define G_2       (F_2*Full_step)
#define A_2       (A_1*2)
#define B_2       (B_1*2)

PaStream* start(tone_table* table) {
    PaStream* result;

    Pa_Initialize();

    Pa_OpenDefaultStream(
        &result, Mono/*in*/, Stereo/*out*/, paFloat32, Sample_rate, Frames_per_buffer,
        my_callback, table
        );
    Pa_StartStream( result );
    return result;
}

void stop(PaStream* stream) {
    Pa_StopStream( stream );
    Pa_CloseStream( stream );
    Pa_Terminate();
}

void report_what_we_heard(tone_table* table) {
    for(int i=0;i<table->tones;i++) {
        printf("%8.3f (%5.3f) was at %10.5f\n",
           table->tone[i].frequency,
           table->tone[i].phase_increment,
           table->tone[i].amplitude_in/100.0
           );
    }
}

void calibrate()
{
    tone_table table;
    table.tones = 0;
    table.samples = 0;

    add_tone(&table,0.0,0.0,A_1*0.996);
    add_tone(&table,0.0,0.0,A_1*0.997);
    add_tone(&table,0.0,0.0,A_1*0.998);
    add_tone(&table,0.0,0.0,A_1*0.999);
    add_tone(&table,0.0,0.0,A_1);
    add_tone(&table,0.0,0.0,A_1*1.001);
    add_tone(&table,0.0,0.0,A_1*1.002);
    add_tone(&table,0.0,0.0,A_1*1.003);
    add_tone(&table,0.0,0.0,A_1*1.004);

    printf("Play you A-below-middle-C for the next ten seconds\n");
    PaStream* stream = start(&table);
    Pa_Sleep( 10 Seconds );
    stop(&stream);
    report_what_we_heard(&table);
}

void slide_test()
{
    tone_table table;
    table.tones = 0;
    table.samples = 0;

    add_tone(&table,1.0,0.0,A_1);
    add_tone(&table,0.0,0.0,B_1);
    add_tone(&table,0.0,0.0,C_2);
    add_tone(&table,0.0,0.0,D_2);
    add_tone(&table,0.0,0.0,E_2);
    add_tone(&table,0.0,1.0,F_2);
    add_tone(&table,0.0,0.0,F_2-A_1);

    PaStream* stream = start(&table);

    for(int i=0;i < 100;i++) {
        table.tone[2].left_amplitude = i*0.01;
        table.tone[5].right_amplitude = (100-i)*0.01;
        Pa_Sleep( 20 Milliseconds );
    }

    Pa_Sleep( 1 Second );

    for(int i=0;i < 100;i++) {
        table.tone[2].left_amplitude = (100-i)*0.01;
        table.tone[5].right_amplitude = i*0.01;
        Pa_Sleep( 20 Milliseconds );
    }

    stop(&stream);
    report_what_we_heard(&table);
}

void chord_test()
{
    tone_table table;
    table.tones = 0;
    table.samples = 0;

    add_tone(&table,2.0,0.0,A_1);
    add_tone(&table,0.5,0.5,C_2);
    add_tone(&table,0.0,1.0,F_2);
    add_tone(&table,0.0,0.0,A_2);

    PaStream* stream = start(&table);
    Pa_Sleep( 5 Seconds );
    table.tone[0].left_amplitude = 0.0;
    table.tone[3].left_amplitude = 1.0;
    Pa_Sleep( 5 Seconds );
    stop(&stream);
    report_what_we_heard(&table);
}

void sing(const char* s) {
    FILE* say = popen("say -v cello","w");
    fprintf(say,s);
    pclose(say);
}

int main(int argc, char**argv)
{
    if(argc != 2) { printf("usage: sound_math <option>\n"); return 1;}

    char* option = argv[1];

    sing("Ready");

    if( strcmp(option, "chord") == 0 )
    {
        chord_test();
    }
    else if( strcmp(option, "calibrate") == 0 )
    {
        printf("calibrate goes here\n");
        /* calibrate(); */
    }
    else if( strcmp(option, "slide") == 0 )
    {
        printf("slide_test goes here\n");
        /* slide_test(); */
    }
    else
    {
        printf("That option (%s) isn't defined yet.\n", option);
    }

    return 0;
}

