#include "stdio.h"
#include "portaudio.h"
#include <math.h>


#define Pi        (3.14159265)
#define Max_tones (10)

typedef struct {
    double           phase;
    double           frequency;
    double           phase_increment;
    double           amplitude;  
      /* TODO: Should have left & right amplitudes */
    volatile double  amplitude_in;
  } wave_form;

typedef struct {
    wave_form        left,right;  
      /*
      TODO: Instead of left, right, this should be an array with 0..Max_tones & a count
      */
  } frequencies_in_and_out;


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
    unsigned int i;

    frequencies_in_and_out *data = (frequencies_in_and_out*)userData;

    data->left.phase_increment  = 0.01;
    data->right.phase_increment = 0.06;
       /* 
       TODO: These should be a constant times the data->xxx.frequency
       TODO: The constant should be in the user data, and represent
         2*Pi/Samples_per_second or something like that
       */

    /* Read input buffer, process data, and fill output buffer. */
    for( i=0; i<framesPerBuffer; i++ ) {
        data_in  = *in++;

        /*
        TODO: this should be a loop over the wave forms
        TODO: instead of appending directly to the channels, this should accumulate a value for each
          and then append them to out.
        TODO: It should compute the cross sections of the data_in with each wave (sin & cos) and then
          update their amplitude_in value after the loop (e.g., in a final loop).  This may require 
          temporary storage somewhere?
        */
        data->left.phase += data->left.phase_increment;
        if( data->left.phase > Pi ) data->left.phase -= 2*Pi;
        *out++ = (float) sin( data->left.phase ) * data->left.amplitude;

        data->right.phase += data->right.phase_increment;
        if( data->right.phase > Pi ) data->right.phase -= 2*Pi;
        *out++ = (float) sin( data->right.phase ) * data->right.amplitude;
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
    frequencies_in_and_out  my_data;
    my_data.left.amplitude = 1.0;
    my_data.right.amplitude = 1.0;
    my_data.left.phase = 0.0;
    my_data.right.phase = 0.0;

    PaStream *stream;

    Pa_Initialize();
    
    Pa_OpenDefaultStream(
        &stream,Mono/*in*/,Stereo/*out*/,paFloat32,Sample_rate,Frames_per_buffer,
        my_callback,&my_data
        );
    Pa_StartStream( stream );
    Pa_Sleep( 10*Seconds );
    Pa_StopStream( stream );
    Pa_CloseStream( stream );


    Pa_Terminate();
    return 0;
}

