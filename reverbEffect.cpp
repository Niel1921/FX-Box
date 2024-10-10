#include "reverbEffect.h"
#include <cmath>

#define DELAY_BUFFER_SIZE 22050

ReverbEffect::ReverbEffect() {
}

ReverbEffect::~ReverbEffect() {
}

bool ReverbEffect::setup() {
    std::vector<int> delayLengths = {1300, 2400, 4000, 4800, 5000, 5600, 6300, 7000, 8100}; // Example lengths in samples
    float baseFeedback = 0.99;  // Base feedback amount
    for(auto length : delayLengths) {
        DelayLine line;
        line.delayLength = length;
        line.feedbackAmount = baseFeedback;
        line.buffer.resize(DELAY_BUFFER_SIZE, 0);
        delayLines.push_back(line);
    }

    return true;
}

void ReverbEffect::process(float in_l, float in_r, float& out_l, float& out_r) {

    // Process each delay for each sample length
    for(int i = 0; i < std::min(activeDelays, static_cast<int>(delayLines.size())); i++) {
        auto& line = delayLines[i];

        // Update write pointer
        if(++line.writePointer >= DELAY_BUFFER_SIZE)
            line.writePointer = 0;
        int readIndex = line.writePointer - line.delayLength;
        if(readIndex < 0) readIndex += DELAY_BUFFER_SIZE;

        // Read following samples
        float delayed_l = line.buffer[readIndex];
        float delayed_r = line.buffer[readIndex];

        // Apply damping to emulate real reverb
        delayed_l *= line.damping;
        delayed_r *= line.damping;

        // Calculate new delay input
        line.buffer[line.writePointer] = in_l + delayed_l * line.feedbackAmount;
        line.buffer[line.writePointer] = in_r + delayed_r * line.feedbackAmount;

        // Accumulate output
        out_l += delayed_l;
        out_r += delayed_r;
    }

    // Mix dry and wet signals
    out_l = in_l * (1 - wetMix) + out_l * wetMix;
    out_r = in_r * (1 - wetMix) + out_r * wetMix;


}


void ReverbEffect::cleanup() {
    // Cleanup code if necessary
}

void ReverbEffect::setWetDryMix(float mix) {
    wetMix = mix;
}

void ReverbEffect::setNumberOfActiveDelays(int count) {
    activeDelays = count;
}

void ReverbEffect::setFeedbackLevel(float level) {
    for(auto& line : delayLines) {
        line.feedbackAmount = level;
    }
}

