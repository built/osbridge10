#include "stdio.h"
#include "portaudio.h"
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#define Pi        (3.14159265)
#define Max_tones (20)
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
    wave_form tone[Max_tones];
    long      samples;
} tone_table;

tone_table table;
PaStream* stream;
int steps = 20;
int step_time = 10 Milliseconds;

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

#define Close_approximation 1e-4

float diff(float f1, float f2) {
    return fabs(f1 - f2);
}

int new_tone(float freq) {
    assert(table.tones < Max_tones); // Range check!

    int i = table.tones;
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

int tone_for(float freq) {
    freq = fabs(freq);

    for(int tone=0;tone<table.tones;tone++)
        if( diff(table.tone[tone].frequency, freq) < Close_approximation) return tone;

    // else capture that frequency in our table.
    return new_tone(freq);
}


void set_volume(int t,float l_amp,float r_amp) {
    table.tone[t].left_amplitude  = l_amp;
    table.tone[t].right_amplitude = r_amp;
}


#define Half_step 1.0594630943593
#define Full_step (Half_step*Half_step)
#define A_1       440.0
#define B_1       (A_1*Full_step)
#define C_2       (B_1*Half_step)
#define C_1       (C_2/2)
#define D_2       (C_2*Full_step)
#define D_1       (D_2/2)
#define E_2       (D_2*Full_step)
#define E_1       (E_2/2)
#define F_2       (E_2*Half_step)
#define F_1       (F_2/2)
#define G_2       (F_2*Full_step)
#define G_1       (G_2/2)
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
    printf("Heterodyne...");
    fflush(stdout);

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
    printf("isolate...");
    fflush(stdout);

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
    for(int t=0;t<table.tones;t++)
        if(t == t1 || t == t2 || t == t3)
            set_volume(t,1.0,1.0);
        else
            set_volume(t,0.0,0.0);
}

void issolate_2(int t1, int t2) {
    issolate_3(t1,t2,No_such_tone);
}

void issolate(int t1) {
    issolate_3(t1,No_such_tone,No_such_tone);
}

void split(float f1,float f2) {
    printf("split...");
    fflush(stdout);

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

float h_add(float a,float b) {
    heterodyne(a,b);
    issolate(tone_for(a+b));
    start_listening();
    Pa_Sleep(steps*step_time);
    table.tones = 0;
    return a+b;
}

float h_sub(float a,float b) {
    heterodyne(a,b);
    issolate(tone_for(a-b));
    start_listening();
    Pa_Sleep(steps*step_time);
    table.tones = 0;
    return a-b;
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

    printf("Play your A-above-middle-C for the next ten seconds\n");
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

#define RET  A_1
#define I    C_1
#define BS   D_1
#define V    E_1
#define ZERO G_1
#define II   C_2
#define ADD  D_2
#define III  G_2
#define EQ   A_2
#define XV   B_2
#define IIII (C_2*2)
#define E    (C_1/4)
#define I5   (II+III)
#define I6   (III+III)
#define I7   (IIII+III)
#define I8   (IIII+IIII)

int add(int a, int b) {
    int a_in = a;
    int b_in = b;
    int result = 0;
    int place = 1;
    int carry = 0;
    while(a != 0 || b != 0 || carry != 0) {
        int lda = a % 10;
        int ldb = b % 10;

        /* In C we'd write: */
/*
        int digit_sum = lda+ldb;
        int carry_out = digit_sum / 10;
        digit_sum = digit_sum % 10;
*/
        /* Instead we replace the above three lines with acoustic math: */

        /* Split the digits to be added into Vs and Is */
        int a_Is = lda % 5;   int a_Vs = lda/5;
        int b_Is = ldb % 5;   int b_Vs = ldb/5;

        /* Convert a_Is, etc. to frequencies f_a_Is, etc. */
        float f_a_Is = a_Is*I;  float f_a_Vs = a_Vs*V;
        float f_b_Is = b_Is*I;  float f_b_Vs = b_Vs*V;

        /* Add the Is & the carry */
        printf("computing %i+%i+%i\n",lda,ldb,carry);
        printf("    %i+%i+%i\n",a_Is,b_Is,carry);
        float R1 = h_add(h_add(h_add(E,f_a_Is),f_b_Is),carry*I);
        printf("\n%f = h_add(h_add(h_add(%f,%f),%f),%f)\n",R1,E,f_a_Is,f_b_Is,carry*I);

        /* Add the Vs and possibly one extra V if the Is sumed to more than 4.5*I */
        float R2 = (R1 > I*4.5) ? h_add(V,E) : E;
        printf("    %i+%i+%i\n",5*a_Vs,5*b_Vs,5*(int) round((R2-E)/V));
        float R3 = h_add(h_add(R2,f_a_Vs),f_b_Vs);
        printf("\n%f = h_add(h_add(%f,%f),%f)\n",R3,R2,f_a_Vs,f_b_Vs);

        /* Convert back to binary because we don't actually have series of tubes */
        int digit_sum_Vs = round(R3/V);
        int digit_sum = round(h_sub(R1,5*I*round(h_sub(R2,E)/V))/I)+5*digit_sum_Vs;

        /* Carry out iff the Vs channel pitch is X or greater */
        int carry_out = digit_sum_Vs/2;

        printf("Sum Vs = %i, sum = %i, carry = %i\n",digit_sum_Vs,digit_sum,carry_out);
        result = result + place*digit_sum;
        place *= 10;
        carry = carry_out;
        a = a/10;
        b = b/10;
    }
    printf("%i + %i = %i\n",a_in,b_in,result);
    return result;
}

#define Threshold 0.1
char listen_for_char() {
    int t_I    = tone_for(I);
    int t_II   = tone_for(II);
    int t_III  = tone_for(III);
    int t_IIII = tone_for(IIII);
    int t_V    = tone_for(V);
    int t_RET  = tone_for(RET);
    int t_BS   = tone_for(BS);
    int t_ADD  = tone_for(ADD);
    int t_EQ   = tone_for(EQ);
    int t_ZERO = tone_for(ZERO);
    int score[Max_tones]; 
    for(int i=0;i<table.tones;i++) score[i] = 0;
    int tones_heard = 0;
    int tones_heard_this_time = 0;
    while(tones_heard == 0 || tones_heard_this_time > 0) {
        start_listening();
        Pa_Sleep( 30 Milliseconds );
        tones_heard_this_time = 0;
        for(int i=0;i<table.tones;i++) {
            float threshold = Threshold*table.tone[i].frequency/440.0;
            if (table.tone[i].amplitude_in > threshold) {
                tones_heard_this_time += 1;
                score[i] += table.tone[i].amplitude_in/threshold;
            }
        }
        tones_heard += tones_heard_this_time;
    }
    int total_score = 0;
    for(int i=0;i<table.tones;i++) {
        total_score += score[i];
        /* printf("%i ",score[i]); */
        }
    int average_score = total_score / table.tones;
    /* printf("  ave = %i\n",average_score); */
    int digit = 0;
    for(int i=0;i<table.tones;i++)
        if (score[i] <= average_score) /* ignore it */;
        else if (i == t_I)     digit += 1;
        else if (i == t_II)    digit += 2;
        else if (i == t_III)   digit += 3;
        else if (i == t_IIII)  digit += 4;
        else if (i == t_V)     digit += 5;
        else if (i == t_RET)   return '\n';
        else if (i == t_BS)    return '\b';
        else if (i == t_ADD)   return '+';
        else if (i == t_EQ)    return '=';
        else if (i == t_ZERO)  return '0';
    return ('0'+digit);
}

char exit_char;
int input_int() {
    int result = 0;
    int digits_seen = 0;
    char ch;
    exit_char = 0;
    while(!exit_char) {
        ch = listen_for_char();

        if      (ch == '\b')             digits_seen -= 1;
        else if (ch >= '0' && ch <= '9') digits_seen += 1;
        if (digits_seen < 0) digits_seen = 0;

        if      (ch == '\b')             result = result / 10;
        else if (ch >= '0' && ch <= '9') result = 10*result + ch - '0';
        else                             exit_char = ch;

        printf("\r%i \b",result);
        fflush(stdout);
    }
    printf("\n");
    exit_char = ch;
    if (digits_seen == 0) return -1;
    return result;
}

int main(int argc, char**argv) {
    if(argc != 2) { printf("usage: sound_math <option>\n"); return 1;}

    char* option = argv[1];

    start();

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
    } else if( strcmp(option, "input") == 0) {
        printf("Play a number and play RET to push; EQ to pop; or <operator>; tone bindings are:\n\
\n\
          middle-C = I\n\
                 D = DEL\n\
                 E = V\n\
                 F\n\
                 G = ZERO\n\
                 A = RETURN\n\
                 B\n\
           tenor-C = II\n\
                 D = ADD\n\
                 E\n\
                 F\n\
                 G = III\n\
                 A = EQ\n\
                 B\n\
            high-C = IIII\n\
\n\
To exit, pop the stack (with EQ) past empty.\n\
For the real deal I plan to have the keys labled.\n\n\
");
       int stack[1000];
       int sp = 0;
       int done = 0;
       sing("Greetings");
       while(!done) {
           int x = input_int();
           if (x >= 0) stack[sp++] = x;
           if (exit_char == '+')
               if (sp < 2) sing("Stack underflow\n");
               else {
                   int b = stack[--sp];
                   int a = stack[--sp];
                   stack[sp++] = add(a,b);
               }
           else if (exit_char == '=') {
             if (sp == 0) done = 1;
             else printf("Popping %i\n",stack[--sp]);
           }
           printf("[");
           for(int i=0;i<sp;i++) printf(" %i ",stack[i]);
           printf("]\n");
       }
       sing("Farewell");
    } else {
        printf("That option (%s) isn't defined yet.\n", option);
    }

    stop();
    report_what_we_heard();
    return 0;
}

