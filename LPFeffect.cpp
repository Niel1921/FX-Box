#include "LPF.h"

void LPF::setup(float sampleRate, float cutoffFrequency, float q) {
    this->sampleRate = sampleRate;
    filter.setup ({
        .fs = sampleRate,
        .type = BiquadCoeff::lowpass,
        .cutoff = cutoffFrequency,
        .q = q,
        .peakGainDb = 3
    });

}


float LPF::process(float in) {
    // Process the input sample through the filter
    
    return filter.process(in);
    
}