#include <Bela.h>
#include <vector>

class ReverbEffect {
public:
    ReverbEffect();
    ~ReverbEffect();

    bool setup();
    void process(float in_l, float in_r, float& out_l, float& out_r);
    void cleanup();

    void setWetDryMix(float mix);
    void setNumberOfActiveDelays(int count);
    void setFeedbackLevel(float level);

private:
    struct DelayLine {
        std::vector<float> buffer;
        int writePointer = 0;
        int delayLength;
        float feedbackAmount;
        float damping = 0.5;  // Simulates high frequency loss
    };

    std::vector<DelayLine> delayLines;
    float wetMix = 0.3; // Default wet/dry mix
    int activeDelays = 3; // Default number of active delays
};

