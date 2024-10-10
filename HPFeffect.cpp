#include "HPF.h"

void HPF::setup(float sampleRate, float cutoffFrequency, float q) {
    this->sampleRate = sampleRate;
    filter.setup ({
        .fs = sampleRate,
        .type = BiquadCoeff::highpass,
        .cutoff = cutoffFrequency,
        .q = q,
        .peakGainDb = 3
    });

}


float HPF::process(float in) {
    // Process the input sample through the filter
    
    return filter.process(in);
    
}