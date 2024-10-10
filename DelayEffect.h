// DelayEffect.h
#ifndef DELAY_EFFECT_H
#define DELAY_EFFECT_H

#include <Bela.h>

#define DELAY_BUFFER_SIZE 44100

class DelayEffect {
public:
    DelayEffect();
    void setup(float sampleRate);
    void setDelayAmount(float amount);
    void setFeedbackAmount(float amount);
    void process(float in_l, float in_r, float& out_l, float& out_r); // Updated signature
    void cleanup();

private:
    float gDelayBuffer_l[DELAY_BUFFER_SIZE] = {0};
    float gDelayBuffer_r[DELAY_BUFFER_SIZE] = {0};
    int gDelayBufWritePtr = 0;
    int gDelayInSamples = 22050; // Example initial value
    float gDelayFeedbackAmount = 0.5; // Example initial value
    float gDelayAmountPre;
    float gDel_a0;
	float gDel_a1;
	float gDel_a2;
	float gDel_a3;
	float gDel_a4;

    float gDel_x1_l, gDel_x2_l, gDel_y1_l, gDel_y2_l;
    float gDel_x1_r, gDel_x2_r, gDel_y1_r, gDel_y2_r;
};

#endif
