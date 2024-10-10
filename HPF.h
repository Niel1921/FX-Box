#include <libraries/Biquad/Biquad.h>

class HPF {
private:
    Biquad filter;
    float sampleRate;

public:
    void setup(float sampleRate, float cutoffFrequency, float q);
    
    float process(float in);
};
