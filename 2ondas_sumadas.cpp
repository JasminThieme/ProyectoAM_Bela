#include <Bela.h>
#include <libraries/Scope/Scope.h> 
#include <cmath> 

const int gWavetableLength = 512;			// The length of the buffer in frames
float triangleWavetable[gWavetableLength];	// Buffer that holds the trianglewavetable
float sineWavetable[gWavetableLength];		// Buffer that holds the sinewavetable
float sawtoothWavetable[gWavetableLength];	// Buffer that holds the sawtooth wavetable
float rectangularWavetable[gWavetableLength];
float* currentTable1 = nullptr;				// Pointer to table
float* currentTable2 = nullptr;				// Pointer to table
float gReadPointer1 = 0, gReadPointer2 = 0;	// Position of the last frame we played 

float gIn1, gIn2, gIn3, gIn4, gIn5;
int gAudioFramesPerAnalogFrame;
float gFrequency1, gFrequency2, gAmplitude;

Scope gScope;

bool setup(BelaContext *context, void *userData)
{
	// Generate a triangle waveform (ramp from -1 to 1, then 1 to -1)
	for(unsigned int n = 0; n < gWavetableLength/2; n++) {
		triangleWavetable[n] = -1.0 + 4.0 * (float)n / (float)gWavetableLength;
	}
	for(unsigned int n = gWavetableLength/2; n < gWavetableLength; n++) {
		triangleWavetable[n] = 1.0 - 4.0 * (float)(n - gWavetableLength/2) / (float)gWavetableLength;
	}
	// Generate a sine waveform
	for(unsigned int n = 0; n < gWavetableLength; n++) {
		sineWavetable[n] = sin( 2 * M_PI * (float)n / (float)gWavetableLength);
	}
	//Generate sawtooth waveform
	for(unsigned int n = 0; n < gWavetableLength; n++) {
		sawtoothWavetable[n] = -1.0 + 2.0 * (float)n / (float)(gWavetableLength - 1);
	}
	//Generate rectangular waveform
	for(unsigned int n = 0; n < gWavetableLength/2; n++) {
		rectangularWavetable[n] = -1.0;
	}
	for(unsigned int n = gWavetableLength/2; n < gWavetableLength; n++) {
		rectangularWavetable[n] = 1.0;
	}

	// Check that we have the expected number of analog inputs and outputs
	// because render() assumes half as many analog frames as audio frames
	if(context->audioFrames != 2*context->analogFrames) {
		rt_fprintf(stderr, "This project needs analog I/O running at half the audio rate.\n");
		return false;
	}
	if(context->analogInChannels < 5) {
		rt_fprintf(stderr, "This project needs at least 5 analog inputs.\n");
		return false;
	}

	//gAudioFramesPerAnalogFrame = 2 , if audioFrames = 44100 and analogFrames = 22050
	if(context->analogFrames)
		gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

	// Initialise the Bela oscilloscope
	gScope.setup(3, context->audioSampleRate);

	return true;
}

void render(BelaContext *context, void *userData)
{
	
	for(unsigned int n = 0; n < context->audioFrames; n++) {
		// read analog inputs and update frequency, amplitude and form
		gIn1 = analogRead(context, n/gAudioFramesPerAnalogFrame, 0); //amplitude
		gIn2 = analogRead(context, n/gAudioFramesPerAnalogFrame, 1); //Frequency1
		gIn3 = analogRead(context, n/gAudioFramesPerAnalogFrame, 2); //waveform1
		gIn4 = analogRead(context, n/gAudioFramesPerAnalogFrame, 3); //frequency2
		gIn5 = analogRead(context, n/gAudioFramesPerAnalogFrame, 4); //waveform2
			
		gAmplitude = map(gIn1, 0, 3.3 / 4.09, -40, -6);
		float amplitude = powf(10, gAmplitude/20);
		gFrequency1 = map(gIn2, 0, 1, 100, 1000);
		gFrequency2 = map(gIn4, 0, 1, 100, 1000);
		//Change waveform depending on positition of the potentiometer
		if(gIn3 <= 0.25){
			currentTable1 = sineWavetable; 
	      // currentTable = &sineWavetable[0];
		} else if (gIn3>0.25 && gIn3 <= 0.50) {
			currentTable1 = triangleWavetable; 
		} else if (gIn3>0.50 && gIn3 <= 0.75) {
			currentTable1 = sawtoothWavetable;  
			amplitude=amplitude/2; //since the sound is more intense
		} else {
			currentTable1 = rectangularWavetable;  
			amplitude=amplitude/2; 
		}
		
		if(gIn5 <= 0.25){
			currentTable2 = sineWavetable; 
	      // currentTable = &sineWavetable[0];
		} else if (gIn5>0.25 && gIn5 <= 0.50) {
			currentTable2 = triangleWavetable; 
		} else if (gIn5>0.50 && gIn5 <= 0.75) {
			currentTable2 = sawtoothWavetable; 
			if(gIn3<0.5)
				amplitude=amplitude/2; 
		} else {
			currentTable2 = rectangularWavetable; 
			if(gIn3<0.5)
				amplitude=amplitude/2; 
		}
		
    	//change float to int bc float doesnt make sense in a buffer
    	int indexbelow = floorf(gReadPointer1); //rounds a float downward
    	int indexabove = indexbelow + 1;
    	if (indexabove >= gWavetableLength)
    		indexabove = 0;
    	//linear interpolation x(n+f) = (1-f)*x[n] + f*x[n+1]
    	float fractionAbove = gReadPointer1 - indexbelow;
    	float fractionBelow = 1.0 - fractionAbove;
    	float pos1 = fractionBelow * currentTable1[indexbelow] + fractionAbove * currentTable1[indexabove];
    	
    	//change float to int bc float doesnt make sense in a buffer
    	int indbelow = floorf(gReadPointer2); //rounds a float downward
    	int indabove = indbelow + 1;
    	if (indabove >= gWavetableLength)
    		indabove = 0;
    	//linear interpolation x(n+f) = (1-f)*x[n] + f*x[n+1]
    	float fractAbove = gReadPointer2 - indbelow;
    	float fractBelow = 1.0 - fractAbove;
    	float pos2 = fractBelow * currentTable2[indbelow] + fractAbove * currentTable2[indabove];
    	
    	// Calculate output
        float out1 = amplitude * pos1;
        float out2 = amplitude * pos2;
        float outsuma = out1+out2;
        
        // Increment read pointer and reset to 0 when end of table is reached
        gReadPointer1 += gWavetableLength*gFrequency1/context->audioSampleRate;
        while(gReadPointer1 >= gWavetableLength) //while instead of if
            gReadPointer1 -= gWavetableLength; //dont reset to 0
            
        gReadPointer2 += gWavetableLength*gFrequency2/context->audioSampleRate;
        while(gReadPointer2 >= gWavetableLength) //while instead of if
            gReadPointer2 -= gWavetableLength; //dont reset to 0
            
    	//audioWrite(context, n, 0, out2);
    	//audioWrite(context, n, 1, out1);
    	for(unsigned int channel = 0; channel < context->audioOutChannels; channel++) {
			// Write the sample to every audio output channel
    		audioWrite(context, n, channel, outsuma);
    	}
    	
    	// Write the sample to the oscilloscope
    	gScope.log(out1, out2, outsuma);
	}
}

void cleanup(BelaContext *context, void *userData)
{

}