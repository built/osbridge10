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

tone_table table;
PaStream* stream;
int steps = 40;
int step_time = 20 Milliseconds;

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
        t->amplitude_in = ( (t->s_cross*t->s_cross) + (t->c_cross*t->c_cross) ) / table->samples;
    }

    return paContinue;
}

void sing(const char* s) {
    FILE* say = popen("say -v cello","w");
    fprintf(say,s);
    pclose(say);
}

int tone_for(float freq) {
    freq = fabs(freq);
    for(int i=0;i<table.tones;i++)
        if(fabs(table.tone[i].frequency-freq) < 1e-4) return i;
    int i = table.tones;
    /* TODO: Range check! */
    table.tones += 1;
    table.tone[i].left_amplitude  = 0.0;
    table.tone[i].right_amplitude = 0.0;
    table.tone[i].amplitude_in    = 0.0;
    table.tone[i].frequency       = freq;
    table.tone[i].phase           = 0.0;
    table.tone[i].s_cross         = 0.0;
    table.tone[i].c_cross         = 0.0;
    table.tone[i].phase_increment = freq*2*Pi/Sample_rate;
    return i;
}

void set_volume(int t,float l_amp,float r_amp) {
    table.tone[t].left_amplitude  = l_amp;
    table.tone[t].right_amplitude = r_amp;
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
#define C_3       (C_2*2)
#define G_3       (G_2*2)


void start() {
    table.tones = 0;
    table.samples = 0;
    Pa_Initialize();
    Pa_OpenDefaultStream(
        &stream, Mono/*in*/, Stereo/*out*/, paFloat32, Sample_rate, Frames_per_buffer,
        my_callback, &table
        );
    Pa_StartStream( stream );
}

void stop() {
    Pa_StopStream( stream );
    Pa_CloseStream( stream );
    Pa_Terminate();
}

void start_listening() {
    table.samples = 0;
    for(int i=0;i<table.tones;i++) {
        table.tone[i].s_cross = 0.0;
        table.tone[i].c_cross = 0.0;
    }
}

void report_what_we_heard() {
    for(int i=0;i<table.tones;i++) {
        printf("%8.3f (%5.3f) was at %10.5f\n",
           table.tone[i].frequency,
           table.tone[i].phase_increment,
           table.tone[i].amplitude_in/100.0
           );
    }
}

void heterodyne(float f1,float f2) {
    printf("Heterodyne\n");

    table.tones = 0;
    int t1     = tone_for(f1);
    int t2     = tone_for(f2);
    int t_sum  = tone_for(f1+f2);
    int t_diff = tone_for(f1-f2);

    for(int i=0;i < steps;i++) {
        float x = i*1.0/steps;
        set_volume(t1,1.0,  x);
        set_volume(t2,  x,1.0);
        Pa_Sleep( step_time );
    }

    for(int i=0;i < steps;i++) {
        float x = i*1.0/steps;
        set_volume(t1,    1.0-x,1.0-x);
        set_volume(t2,    1.0-x,1.0-x);
        set_volume(t_sum,     x,0.01 );
        set_volume(t_diff,0.01 ,    x);
        Pa_Sleep( step_time );
    }
}

#define No_such_tone (-1)
void issolate_3(int t1, int t2, int t3) {
    float ratio = 0.9;
    for(int i=0;i < steps;i++) {
        for(int t=0;t<table.tones;t++){
            wave_form* w = &table.tone[t];
            if(t == t1 || t == t2 || t == t3) {
                w->left_amplitude  = 1.0 - (1.0-w->left_amplitude )*ratio;
                w->right_amplitude = 1.0 - (1.0-w->right_amplitude)*ratio;
            } else {
                w->left_amplitude  *= ratio;
                w->right_amplitude *= ratio;
            }
        }
    }
}

void issolate_2(int t1, int t2) {
    issolate_3(t1,t2,No_such_tone);
}

void issolate(int t1) {
    issolate_3(t1,No_such_tone,No_such_tone);
}

void split(float f1,float f2) {
    printf("split\n");
    int t1 = tone_for(f1);
    int t2 = tone_for(f2);
    issolate_2(t1,t2);

    for(int i=0;i < steps;i++) {
        float x = i*1.0/steps;
        set_volume(t1,1.0,1.0-x);
        set_volume(t2,1.0-x,1.0);
        Pa_Sleep( step_time );
    }
}

void h_add(float a,float b) {
    heterodyne(a,b);
    issolate(tone_for(a+b));
    start_listening();
    Pa_Sleep(steps*step_time);
}

void h_sub(float a,float b) {
    heterodyne(a,b);
    issolate(tone_for(a-b));
    start_listening();
    Pa_Sleep(steps*step_time);
}

void calibrate() {
    table.tones = 0;
    tone_for(A_1*0.996);
    tone_for(A_1*0.997);
    tone_for(A_1*0.998);
    tone_for(A_1*0.999);
    tone_for(A_1);
    tone_for(A_1*1.001);
    tone_for(A_1*1.002);
    tone_for(A_1*1.003);
    tone_for(A_1*1.004);

    printf("Play your A-below-middle-C for the next ten seconds\n");
    Pa_Sleep( 10 Seconds );
}

void chord_test() {
    table.tones = 0;
    set_volume(tone_for(A_1),2.0,0.0);
    set_volume(tone_for(C_2),0.5,0.5);
    set_volume(tone_for(F_2),0.0,1.0);

    Pa_Sleep( 5 Seconds );
    set_volume(tone_for(A_1),0.0,0.0);
    set_volume(tone_for(A_2),1.0,0.0);
    Pa_Sleep( 5 Seconds );
}

#define I    C_2
#define II   C_3
#define III  G_4
#define IIII C_4
#define V    E_2
#define X    E_3
#define XV   B_3
#define E    Full_step

int add(int a, int b) {

    return 0;

}


int main(int argc, char**argv) {
    if(argc != 2) { printf("usage: sound_math <option>\n"); return 1;}

    char* option = argv[1];

    start();
    /* sing("Ready"); */

    if( strcmp(option, "chord") == 0 ){
        printf("Chord test.\n");
        chord_test();
    } else if( strcmp(option, "calibrate") == 0 ) {
        printf("Calibrate.\n");
        calibrate();
    } else if( strcmp(option, "heterodyne") == 0 ) {
        printf("Heterodyne test.\n");
        h_sub(A_1,C_3);
        h_add(A_1,C_3);
    } else {
        printf("That option (%s) isn't defined yet.\n", option);
    }

    stop();
    report_what_we_heard();
    return 0;
}

