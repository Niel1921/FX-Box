/*
 -------------------------------------------------------------
| DELAY FUNCTIONALITY ADAPTED FROM Bela/examples/Audio/delay/ |
 -------------------------------------------------------------
*/


#include "DelayEffect.h"
#define DELAY_BUFFER_SIZE 44100

DelayEffect::DelayEffect() {

	
}

void DelayEffect::setup(float sampleRate) {
}

// Set the amount of delay
void DelayEffect::setDelayAmount(float amount) {

    gDelayInSamples = static_cast<int>(amount * DELAY_BUFFER_SIZE);
}

// Set the amount of feedback
void DelayEffect::setFeedbackAmount(float amount) {
    gDelayFeedbackAmount = amount;
}

// Process the delay effect
void DelayEffect::process(float in_l, float in_r, float& out_l, float& out_r) {
    if (++gDelayBufWritePtr >= DELAY_BUFFER_SIZE) {
        gDelayBufWritePtr = 0;
    }

    // Calculate the delayed input with feedback
    float del_input_l = in_l + gDelayBuffer_l[(gDelayBufWritePtr - gDelayInSamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE] * gDelayFeedbackAmount;
    float del_input_r = in_r + gDelayBuffer_r[(gDelayBufWritePtr - gDelayInSamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE] * gDelayFeedbackAmount;

    // Write the input to the delay buffer
    gDelayBuffer_l[gDelayBufWritePtr] = del_input_l;
    gDelayBuffer_r[gDelayBufWritePtr] = del_input_r;

    // Read the output from the delay buffer
    out_l = gDelayBuffer_l[(gDelayBufWritePtr - gDelayInSamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE];
    out_r = gDelayBuffer_r[(gDelayBufWritePtr - gDelayInSamples + DELAY_BUFFER_SIZE) % DELAY_BUFFER_SIZE];
}


void DelayEffect::cleanup() {
	
}
