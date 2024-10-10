#include <Bela.h>
#include <cmath>
#include <libraries/Trill/Trill.h>
#include <libraries/AudioFile/AudioFile.h>
#include <signal.h>
#include <libraries/Oscillator/Oscillator.h>
#include <unistd.h>
#include "u8g2/U8g2LinuxI2C.h"
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include "DelayEffect.h"
#include "HPF.h"
#include "LPF.h"
#include "reverbEffect.h"

//Initialise each effect class
DelayEffect delayEffect;
HPF highPassFilter;
LPF lowPassFilter;
LPF delayLPF;
Oscillator myOscillator;
ReverbEffect reverb;

//Initialise vlaues for recording function
std::vector<std::vector<float>> gInputs;
std::vector<std::vector<float>> gOutputs;
std::string gFilenameOutputs = "outputs.wav";
std::string gFilenameInputs = "inputs.wav";
const double gDurationSec = 10;  
unsigned int gWrittenFrames = 0;

//Bitmaps for logos/drawings for OLED
static const unsigned char image_arrow_right_bits[] U8X8_PROGMEM = {0x10,0x20,0x7f,0x20,0x10};
static const unsigned char image_menu_home_bits[] U8X8_PROGMEM = {0x7f,0x7f,0xb7,0x7e,0xc7,0x7d,0x67,0x7b,0xb7,0x76,0xdb,0x6d,0xed,0x5b,0xf6,0x37,0x99,0x4f,0x9b,0x6f,0xfb,0x68,0xfb,0x6a,0xfb,0x68,0xfb,0x6a,0x03,0x60};
static const unsigned char image_checked_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x30,0x00,0x38,0x01,0x1c,0x03,0x0e,0x07,0x07,0x8e,0x03,0xdc,0x01,0xf8,0x00,0x70,0x00,0x20,0x00,0x00,0x00,0x00,0x00};
static const unsigned char image_crossed_bits[] U8X8_PROGMEM = {0x00,0x00,0x00,0x00,0x03,0x06,0x07,0x07,0x8e,0x03,0xdc,0x01,0xf8,0x00,0x70,0x00,0xf8,0x00,0xdc,0x01,0x8e,0x03,0x07,0x07,0x03,0x06,0x00,0x00,0x00,0x00,0x00,0x00};
static const unsigned char image_Sine_Wave__PSF___1__bits[] U8X8_PROGMEM = {0x00,0x00,0x20,0x00,0x00,0x78,0x00,0xf8,0x00,0x00,0xfc,0x00,0xf8,0x01,0x00,0xee,0x01,0x9c,0x03,0x00,0xce,0x01,0x9c,0x03,0x00,0x86,0x01,0x0c,
																			0x07,0x00,0x87,0x03,0x0e,0x07,0x00,0x87,0x03,0x0e,0x06,0x00,0x03,0x03,0x06,0x06,0x0c,0x03,0x07,0x07,0x0e,0x0e,0x03,0x07,0x07,0x0e,0x0e,0x00,
																			0x07,0x03,0x0c,0x06,0x00,0x06,0x03,0x1c,0x07,0x00,0x8e,0x03,0x1c,0x07,0x00,0x8c,0x03,0x18,0x03,0x00,0xdc,0x01,0xf8,0x03,0x00,0xf8,0x01,0xf0,0x01,0x00,0xf0,0x00,0xe0,0x00};

//Main navigation enums and boolean checks
enum MenuOptions {
    MENU_REVERB,
    MENU_DELAY,
    MENU_LPF,
    MENU_HPF,
    MENU_TREMOLO,
    MENU_SIZE 
};

enum AppState {
    START_MENU,
    PLAY_MODE,
    VOICE_MODE,
    MAINMENU_SIZE
};

enum WaveTables {
    SAW_WAVE,
    SINE_WAVE,
    SQUARE_WAVE,
    WAVETABLE_SIZE
};

volatile int gMainState = START_MENU;
volatile int gWaveState = SAW_WAVE;
volatile int gMenuState = MENU_REVERB;
volatile bool gButton1Pressed = false;
volatile bool gButton2Pressed = false;
volatile bool gButton3Pressed = false;
int current1ButtonState = 1;
int current2ButtonState = 1;
int current3ButtonState = 0;
int last1ButtonCheck;
int last2ButtonCheck;
int last3ButtonCheck;
int gInputPin1 = 0;
int gInputPin2 = 1;
int gInputPin3 = 2;
bool viewParamFeed = false;
bool recordMode = false;
bool curReccording = false;
bool recComplete = false;
bool saveRec = false;


//Base value initialisation for FX
float lastDelayAmount = 0.5; // Default or initial value
float lastFeedbackAmount = 0.5;
unsigned int gSecondsElapsed = 0;
unsigned int gLastSecond = 0;
bool delayEffectActive = false;
float gPhase;
float lfoAmplitude = 0.1; 
float gInverseSampleRate;


//Values for reverb visualizer
struct Particle {
    float x, y;  // Position
    float vx, vy;  // Velocity
    bool active = false;
};
std::vector<Particle> particles;
const int displayWidth = 128;
const int displayHeight = 64;

//Touch sensor declaration
std::vector<Trill*> gTouchSensors;
AuxiliaryTask oledUpdateTask;
Trill touchSensor;
float gTouchPosition[2] = { 0.0 , 0.0 };
float gTouchSize = 0.0;

//OLED initialisation
const unsigned int gI2cBus = 1;
#define DELAY_BUFFER_SIZE 44100
unsigned int gActiveTarget = 0;

struct Display {U8G2 d; int mux;};
std::vector<Display> gDisplays = {
	{ U8G2LinuxI2C(U8G2_R0, gI2cBus, 0x3c, u8g2_Setup_ssd1306_i2c_128x64_noname_f), -1 },
};


//Background function to write recording to disk and reset the buffer for new recording
void saveRecordingAndReset(bool agree) {
	if(agree){
		// Save recording if correct button pressed
    	AudioFileUtilities::write(gFilenameOutputs, gOutputs, 44100);
	}

    // Clear recording buffers
    for(auto& channel : gOutputs) {
        std::fill(channel.begin(), channel.end(), 0.0f); // Optional: Clear buffer data
    }
    
    // Reset frame counter and navigation flags
    gWrittenFrames = 0;
    recComplete = false;
	curReccording = false;
	saveRec = false;
}

/*
   -----------------------------------------------------------
  | MAIN AUXILIARY LOOP FOR BUTTON AND SENSOR LOGIC/DETECTION |   
   -----------------------------------------------------------
*/

void loop(void*)
{
    while(!Bela_stopRequested()) {

        if(gButton1Pressed) {
        	
        	if(gMainState == VOICE_MODE){ 
        		
        		//Logic for Recording functionality
        		if(recordMode && !curReccording && !recComplete){
        			
        			recordMode = false;
        		}
        		else if(recordMode && !curReccording && recComplete){

        			saveRecordingAndReset(false);
        			recordMode = false;
        		}
        		else if(!recordMode){
        			gMainState = START_MENU;
        		}
        		
        		
        	}
        	
        	else if(gMainState == PLAY_MODE){
        		
	        	gMenuState = (gMenuState + 1) % MENU_SIZE;
	        }
	        
			else{

	            gMainState = PLAY_MODE;
	            Bela_setHeadphoneLevel(-40.0); //Reduce volume for the oscillator

			}
            
            //Reset the button press flag
            gButton1Pressed = false;
        }
        
     
        
        if(gButton2Pressed) {
        	
        	if(gMainState == VOICE_MODE){
        		//Negative options for Recording
        		if(recordMode && !curReccording && !recComplete){
        			
        			curReccording = true;
        		}
        		
        		else if(recordMode && !curReccording && recComplete){
        			saveRec = true;
        		}
        		
        		else if(!recordMode){

		    		gMenuState = (gMenuState + 1) % MENU_SIZE;

        		}
	        }
	        
	        else if(gMainState == PLAY_MODE){
	        
	        	gWaveState = (gWaveState + 1) % WAVETABLE_SIZE;
	        }
	        
	        else{
	        	gMainState = VOICE_MODE;
	        	Bela_setHeadphoneLevel(8.0); //Increase volume as microphone output far lower than oscillator
	        }
            
            gButton2Pressed = false;
        }
        
       
        if(gButton3Pressed) {

            if(gMainState == VOICE_MODE){ 
	    		recordMode = true;
	        }
            else if(gMainState == PLAY_MODE){
	        	gMainState = START_MENU;
	        }
            
            gButton3Pressed = false;
        }
        
        // Read values for trill sensor

        for(unsigned int n = 0; n < gTouchSensors.size(); ++n)
		{
			Trill* t = gTouchSensors[n];
			t->readI2C();
		}
		

    	if(recComplete && saveRec){
        	
        	//Call Recording saving within auxiliary task to reduce load on audio thread
        	
			saveRecordingAndReset(true);

	    }
    	
    	usleep(20000); //Reduce rate to recuce CPU load (20ms delay)
    	
    	
    }
}


/*
   ---------------------------
  | MAIN VISUALIZER FUNCTIONS |
   ---------------------------
*/


void drawSensorInputAsBar(U8G2 &u8g2, int xPos ,float barVal) {
    
    //Draw vertical bar mapped to touch sensor
    int displayHeight = static_cast<int>(17 + 30 * (1-barVal));

    u8g2.drawBox(xPos, displayHeight, 10, 47 - displayHeight);
    
}


//Initialise base particle vector
void initParticles(int count) {
    particles.resize(count);
    for(auto& p : particles) {
        p.active = false;
    }
}

void updateParticles(int numActiveDelays, float feedback) {
    int maxActive = (numActiveDelays * particles.size()) / 9;
    float boxSize = feedback * 30 + 30;  // Box grows with feedback

	//Loop to draw all particles within the range of active particles
	
    for(int i = 0; i < maxActive; ++i) {
    	
        auto& particle = particles[i];
        
        if(!particle.active) {
            // Reactivate particles within the box when deleted
            particle.x = (rand() % (int)boxSize) + (displayWidth - boxSize) / 2;
            particle.y = (rand() % (int)boxSize) + (displayHeight - boxSize) / 2;
            particle.vx = (rand() % 3 - 1) * feedback;  // Particle speed varies with feedback
            particle.vy = (rand() % 3 - 1) * feedback;
            particle.active = true;
        }

        // Move particle based on random velocity generated
        particle.x += particle.vx;
        particle.y += particle.vy;

        // Delete particles when they travel out of the box 
        if(particle.x < (displayWidth - boxSize) / 2 || particle.x > (displayWidth + boxSize) / 2 ||
           particle.y < (displayHeight - boxSize) / 2 || particle.y > (displayHeight + boxSize) / 2) {
            particle.active = false;  // Deactivate particles
        }
    }

    // Deactivate particles outside of the given range
    for(int i = maxActive; i < particles.size(); ++i) {
        particles[i].active = false;
    }
}



void drawReverbVisualizer(U8G2 &u8g2, int numActiveDelays, float feedback){
	
	//Draw the variable box and call the particle update function
	float boxSize = feedback * 30 + 30;  // Box grows with feedback

    int boxX = 20+(displayWidth - boxSize) / 2;
    int boxY = (displayHeight - boxSize) / 2;
    u8g2.drawFrame(boxX, boxY, boxSize, boxSize);

    
    updateParticles(numActiveDelays, feedback);
    for(auto& p : particles) {
        if(p.active) {
            u8g2.drawPixel(p.x+20, p.y);  // Draw particle on the OLED
        }
    }
}


void drawDelayVisualizer(U8G2 &u8g2, float preDelayNormalised, float delayAmountNormalised, int maxEchoes){
    int displayWidth = u8g2.getDisplayWidth();
    int displayHeight = u8g2.getDisplayHeight();
    int baseY = displayHeight - 10;
    int maxAmplitude = 30; 

    // Calculate line spacing based on preDelayNormalised
    int lineSpacing = static_cast<int>(5 + 6 * preDelayNormalised); // Adjust line spacing between 10 and 40 pixels

    // Simulating echos with fading successive lines
    for (int i = 0; i < maxEchoes; i++) {
        float decayFactor = pow(delayAmountNormalised, i);
        int echoAmplitude = static_cast<int>(3 + 0.8 * maxAmplitude * decayFactor);

        // Calculate the vertical position for line
        int yPos = baseY - i * lineSpacing;

        // Finishes drawing when display height has been reached
        if (yPos < lineSpacing){
        	
        	break;
        } 

        u8g2.drawLine((40+displayWidth) / 2 - echoAmplitude, yPos, (40+displayWidth) / 2 + echoAmplitude, yPos);
    }
}




void drawLowPassFilterGraph(U8G2 &u8g2, float cutoffFreqNormalized, float qFactorNormalized) {
	//draw within defined box
    int displayWidth = 60;
    int displayHeight = 40;
    int startY = displayHeight;

    // Calculate the x position of the cutoff frequency within defined range
    int cutoffX = static_cast<int>(min(0.25+cutoffFreqNormalized,0.9) * displayWidth);

    // Calculate peak height based on Q factor
    int peakHeight = static_cast<int>(qFactorNormalized * displayHeight / 2.5); 

    // Draw the box to represent values until cutoff frequency
    u8g2.drawBox(49,startY,cutoffX,24);

    // Draw the peak using multiple lines for thickness
    for(int i = -12; i <= peakHeight; ++i) {
    	int lineLength;
    	if (i<=0){
    		lineLength = peakHeight - 3i + (i^2) ; //Line length begins with an increase...
    	}
    	else{
    		lineLength = peakHeight - 6i - (i^2) ; //...then decreases once the peak height has been reached
    	}
    	
        u8g2.drawVLine(60+max(cutoffX + i,0), min(startY - lineLength, startY), startY+ lineLength); // Draw the current peak value
    }

    // Find the x position for the end of the peak
    int endOfPeakX = 60 + cutoffX + peakHeight;

	//draw lines to prevent steep loooking drop after the end of the peak
	u8g2.drawVLine(endOfPeakX, startY+1, 40); 
	u8g2.drawVLine(endOfPeakX+1, startY+5, 40); 
	u8g2.drawVLine(endOfPeakX+2, startY+9, 40); 


}


void drawHighPassFilterGraph(U8G2 &u8g2, float cutoffFreqNormalized, float qFactorNormalized) {
	//Same logic as the previous filter graph, however mirrored to represent the HPF graph
	
    int displayWidth = 60;
    int displayHeight = 40;
    int startY = displayHeight;

    int cutoffX = static_cast<int> (cutoffFreqNormalized * displayWidth);

    int peakHeight = static_cast<int>(qFactorNormalized * displayHeight / 2.5);

    u8g2.drawBox(65+cutoffX,startY,128,24);

    for(int i = -12; i <= peakHeight; ++i) {
    	int lineLength;
    	if (i<=0){
    		lineLength = peakHeight - 3i + (i^2) ; 
    	}
    	else{
    		lineLength = peakHeight - 6i - (i^2) ; 
    	}
    	
        u8g2.drawVLine(max(65+cutoffX + i,0), min(startY - lineLength, startY), startY+ lineLength); 
    }

    int endOfPeakX = 52+(cutoffX);
	u8g2.drawVLine(endOfPeakX, startY+1, 40); 
	u8g2.drawVLine(endOfPeakX-1, startY+5, 40); 
	u8g2.drawVLine(endOfPeakX-2, startY+9, 40); 


}



void drawTremoloVisualizer(U8G2 &u8g2, float pitchVarNormalised, float rateNormalised){
	
	int displayWidth = 128;
    int displayHeight = 86;
    
    float amplitude = 2 + pitchVarNormalised * (displayHeight / 6);
    float frequency = 2 + rateNormalised * 6.0;
    int numPoints = 100; 
    int previousX = 0;
    int previousY = (int)(displayHeight / 2);

    // Draw a sine wave with the maximum height within the box bounds
    for(int i = 0; i <= numPoints; i++) {
    	
    	//Iterate through each point and calculate the next value
        float phase = ((i) / (float)numPoints) * 2 * M_PI * frequency;
        int x = (int)(((i) / (float)numPoints) * displayWidth);
        int y = (int)(amplitude * sin(phase) + (displayHeight / 2));

        // Draw line from previous point to current point prevously calculated
        
        u8g2.drawLine(previousX, previousY, x, y);
        
        // Update previous point for next iteration
        previousX = x;
        previousY = y;
    }
	
}


void drawTimer(U8G2 &u8g2){
	
	
    gSecondsElapsed++; // Increment our timer every second
    if (gSecondsElapsed >= 10) { // Reset after reaching 10 seconds
        gSecondsElapsed = 0;
        recComplete = true;
    }
    
    char timeStr[2];  // Build string of timer

	snprintf(timeStr, sizeof(timeStr), "%d", gSecondsElapsed);  // Convert the digit to a string to draw

    u8g2.drawStr(44, 37, timeStr);

}

/*
   -------------------------------------
  | MAIN PROCESSING FOR MENU INTERFACES |
   -------------------------------------
*/

void drawVoiceOutline(U8G2 &u8g2, bool viewParam, float xVal, float yVal, float barVal){
    u8g2.setFontMode(1);
	u8g2.setBitmapMode(1);
	
	u8g2.drawFrame(-1, -1, 35, 13);
	u8g2.setFont(u8g2_font_profont12_tr);
	u8g2.drawStr(1, 1, "Voice");
	if(!viewParam){
		
		//Outline of general menu without parameter view
		u8g2.drawXBM(113, 54, 7, 5, image_arrow_right_bits);
		u8g2.drawXBM(3, 48, 15, 15, image_menu_home_bits);
		u8g2.drawFrame(108, 48, 17, 16);
		u8g2.drawLine(127, 11, 34, 11);
		u8g2.setFont(u8g2_font_5x7_tr);
		u8g2.drawStr(38, 2, "(Touch to view)");
	}

	if(!recordMode){
		
/*
	   ------------------------------------------------
	  | MAIN SWITCH CASE FOR PARAMTER VIEW AND FX MENU |
	   ------------------------------------------------
*/
		switch(gMenuState) {
			//General geometry for parameter feed outlines found in each case
	        case MENU_REVERB:
	        
	        	if(viewParamFeed){
	        		u8g2.clearBuffer();
	        		u8g2.drawFrame(-1, -1, 35, 15);
					u8g2.setFont(u8g2_font_profont12_tr);
					u8g2.drawVLine(33, 13, 50);
					u8g2.drawStr(1, 1, "Voice");
					u8g2.drawStr(6, 50, "Mix");
	        		drawReverbVisualizer(u8g2, 3 + (xVal * 6), yVal);
	        		drawSensorInputAsBar(u8g2, 10, barVal);
	        		u8g2.sendBuffer();
	        		
	        	}
	        	else{
	        		u8g2.setFont(u8g2_font_profont29_tr);
					u8g2.drawStr(16, 20, "REVERB");
	        	}
			
	            break;
	            
	        case MENU_DELAY:
	        	
	        	if(viewParamFeed){
	        		u8g2.clearBuffer();
	        		drawDelayVisualizer(u8g2, xVal, yVal, 7);
	        		u8g2.drawFrame(-1, -1, 35, 15);
					u8g2.setFont(u8g2_font_profont12_tr);
					u8g2.drawVLine(33, 13, 50);
					u8g2.drawStr(1, 1, "Voice");
					u8g2.drawStr(6, 50, "Mix");
	        		drawSensorInputAsBar(u8g2, 10, barVal);
	        		u8g2.sendBuffer();
	        	}
	        	else{
	        		u8g2.setFont(u8g2_font_profont29_tr);
					u8g2.drawStr(20, 20, "DELAY");
	        	}
				
	            break;
	            
	        case MENU_LPF:
				
				if(viewParamFeed){
					u8g2.clearBuffer();
					u8g2.drawFrame(-1, -1, 35, 15);
					u8g2.setFont(u8g2_font_profont12_tr);
					u8g2.drawFrame(49, 13, 79, 52);
					u8g2.drawLine(0, 13, 127, 13);
					u8g2.drawStr(1, 1, "Voice");
					u8g2.drawStr(14, 50, "Mix");
					drawLowPassFilterGraph(u8g2, xVal, yVal);
					drawSensorInputAsBar(u8g2, 17, barVal);
					u8g2.sendBuffer();
				}
				else{
					u8g2.setFont(u8g2_font_profont17_tr);
					u8g2.drawStr(28, 19, "LOW PASS");
					u8g2.drawStr(28, 34, "FILTER");
				}
	            break;
	            
	        case MENU_HPF:
	        	if(viewParamFeed){
					u8g2.clearBuffer();
					u8g2.drawFrame(-1, -1, 35, 15);
					u8g2.setFont(u8g2_font_profont12_tr);
					u8g2.drawFrame(49, 13, 79, 52);
					u8g2.drawLine(0, 13, 127, 13);
					u8g2.drawStr(1, 1, "Voice");
					u8g2.drawStr(14, 50, "Mix");
					drawHighPassFilterGraph(u8g2, xVal, yVal);
					drawSensorInputAsBar(u8g2, 17, barVal);
					u8g2.sendBuffer();
				}
				else{
					u8g2.setFont(u8g2_font_profont17_tr);
					u8g2.drawStr(25, 19, "HIGH PASS");
					u8g2.drawStr(25, 34, "FILTER");
				}
	            break;
	            
	    	case MENU_TREMOLO:
				
				
				if(viewParamFeed){
					u8g2.clearBuffer();
					u8g2.drawFrame(-1, -1, 35, 15);
					u8g2.setFont(u8g2_font_profont12_tr);
					
					u8g2.drawLine(0, 13, 127, 13);
					u8g2.drawStr(1, 1, "Voice");
					
					drawTremoloVisualizer(u8g2, yVal, xVal);
					
					u8g2.sendBuffer();
					
				}
				else{
					u8g2.setFont(u8g2_font_profont29_tr);
					u8g2.drawStr(8, 20, "TREMOLO");
				}
				
				
	    		break;
		}
	}
	
	//Main display intructions for recording menu
	
	else if(recordMode && !curReccording && !recComplete){
		u8g2.clearBuffer();
		u8g2.setFont(u8g2_font_profont17_tr);
		u8g2.drawStr(14, 7, "RECORD THIS");
		u8g2.drawStr(14, 23, "EFFECT?");
		u8g2.drawXBM(108, 46, 14, 16, image_checked_bits);
		u8g2.drawXBM(6, 46, 11, 16, image_crossed_bits);
		u8g2.sendBuffer();
	}
	
	else if(recordMode && (curReccording || recComplete)){
		u8g2.clearBuffer();
		u8g2.setFont(u8g2_font_profont22_tr);
		u8g2.drawStr(10, 15, "RECORDING");
		u8g2.setFont(u8g2_font_profont17_tr);
		u8g2.drawStr(56, 37, "/10");
		drawTimer(u8g2);
		
		if(recComplete){
			u8g2.clearBuffer();
			u8g2.setFont(u8g2_font_profont22_tr);
			u8g2.drawStr(10, 15, "SAVE?");
			u8g2.drawXBM(108, 46, 14, 16, image_checked_bits);
			u8g2.drawXBM(6, 46, 11, 16, image_crossed_bits);
			
			if(saveRec){
				u8g2.clearBuffer();
				u8g2.setFont(u8g2_font_profont22_tr);
				u8g2.drawStr(10, 15, "SAVED!!");
				recordMode = false;
				curReccording = false;
				recComplete = false;
			}

			usleep(100000);

		}
		
		//delays calling of recording interface to increment counter
		u8g2.sendBuffer();
		usleep(1000000);
		
	}
	
}

void drawPlayOutline(U8G2 &u8g2, bool viewParam, float xVal, float yVal, float barVal){
	
	//Basic outlines and text for Play Mode to cycle through
	
	u8g2.setFontMode(1);
	u8g2.setBitmapMode(1);
	
	//Basic outline that each variable string can use
	u8g2.drawFrame(46, -2, 36, 16);
	u8g2.setFont(u8g2_font_profont12_tr);
	u8g2.drawStr(52, 1, "PLAY");
	u8g2.drawFrame(63, 13, 2, 51);
	u8g2.drawStr(93, 1, "WAVE");
	u8g2.drawStr(18, 1, "FX");
	u8g2.drawLine(0, 13, 127, 13);
	u8g2.drawXBM(4, 55, 7, 5, image_arrow_right_bits);
	u8g2.drawXBM(116, 55, 7, 5, image_arrow_right_bits);
	
	switch(gMenuState) {
        case MENU_REVERB:
			u8g2.setFont(u8g2_font_profont17_tr);
			u8g2.drawStr(3, 28, "REVERB");
            break;
            
        case MENU_DELAY:
			u8g2.setFont(u8g2_font_profont17_tr);
			u8g2.drawStr(8, 28, "DELAY");
            break;
            
        case MENU_LPF:
			u8g2.setFont(u8g2_font_profont11_tr);
			u8g2.drawStr(14, 18, "LOW");
			u8g2.drawStr(14, 29, "PASS");
			u8g2.drawStr(14, 40, "FILTER");
            break;
            
        case MENU_HPF:
        	u8g2.setFont(u8g2_font_profont11_tr);
			u8g2.drawStr(14, 18, "HIGH");
			u8g2.drawStr(14, 29, "PASS");
			u8g2.drawStr(14, 40, "FILTER");
            break;
            
    	case MENU_TREMOLO:
			u8g2.setFont(u8g2_font_profont11_tr);
			u8g2.drawStr(8, 28, "TREMOLO");
    		break;
	}
}


/*
 -----------------------------
| MAIN DISPLAY AUXILIARY TASK |
 -----------------------------
*/

void oledUpdateLoop(void*){
	while(!Bela_stopRequested()) {
		
		//Call display
		U8G2& u8g2 = gDisplays[gActiveTarget].d;

		switch(gMainState){
			
			case START_MENU:
			
				//Display outline for start menu
				u8g2.clearBuffer();
				u8g2.setFontMode(1);
				u8g2.setBitmapMode(1);
				u8g2.setFont(u8g2_font_t0_22b_tr);
				u8g2.drawStr(29, 0, "FX-Box");
				u8g2.drawFrame(3, 45, 41, 17);
				u8g2.drawLine(0, 20, 125, 20);
				u8g2.setFont(u8g2_font_t0_16b_tr);
				u8g2.drawStr(7, 47, "Play");
				u8g2.setFont(u8g2_font_profont15_tr);
				u8g2.drawStr(27, 23, "MODE SELECT");
				u8g2.drawFrame(77, 45, 48, 17);
				u8g2.setFont(u8g2_font_t0_16b_tr);
				u8g2.drawStr(81, 47, "Voice");
				u8g2.sendBuffer();
	            
	            usleep(500000);
	            break;
				
		
			case VOICE_MODE:
		        if(viewParamFeed) {
		        	//Get touch sensor values to pass into the parameter view
		            float xVal = gTouchSensors[1]->compoundTouchHorizontalLocation();
		            float yVal = gTouchSensors[1]->compoundTouchLocation();
		            float barVal = 1-gTouchSensors[0]->compoundTouchLocation();
		
					//Prevent errenous input below a certain size
					if(gTouchSensors[1]->compoundTouchSize() < 0.05){
		            	xVal = 0;
		            	yVal = 0;
					}
					
					u8g2.clearBuffer();
					drawVoiceOutline(u8g2, viewParamFeed, xVal, yVal, barVal);
					u8g2.sendBuffer();
		    		
		            viewParamFeed= false;
		            usleep(15000); 
		        }
		        else{

		        	u8g2.clearBuffer();
		        	drawVoiceOutline(u8g2, viewParamFeed, 0, 0, 0);
		        	u8g2.sendBuffer();
		        	usleep(200000);
		        	
		        }
		        break;
		        
	        case PLAY_MODE :
	        	
	        	float xVal = gTouchSensors[1]->compoundTouchHorizontalLocation();
	            float yVal = gTouchSensors[1]->compoundTouchLocation();
	            float barVal = 1-gTouchSensors[0]->compoundTouchLocation();
	
	
				if(gTouchSensors[1]->compoundTouchSize() < 0.05){
	            	xVal = 0;
	            	yVal = 0;
				}
	        
    			u8g2.clearBuffer();
	            drawPlayOutline(u8g2, viewParamFeed, xVal, yVal, barVal);
	            
	            switch(gWaveState){
	            	//Draw representations for each Waveform
	            	
	            	case SAW_WAVE:
	            	
	            		u8g2.drawLine(64, -1, 64, 63);
						u8g2.drawLine(71, 35, 86, 20);
						u8g2.drawLine(87, 20, 87, 35);
						u8g2.drawLine(88, 35, 103, 20);
						u8g2.drawLine(104, 20, 104, 35);
						u8g2.drawLine(105, 35, 119, 21);
						u8g2.setFont(u8g2_font_profont11_tr);
						u8g2.drawStr(89, 44, "SAW");
						myOscillator.setType(Oscillator::sawtooth);
						
	            		break;
	            	
	            	case SINE_WAVE:
	            		
	            		u8g2.setBitmapMode(1);
						u8g2.drawLine(64, -1, 64, 63);
						u8g2.drawXBM(78, 21, 36, 18, image_Sine_Wave__PSF___1__bits);
						u8g2.setFont(u8g2_font_profont11_tr);
						u8g2.drawStr(84, 44, "SINE");
						myOscillator.setType(Oscillator::sine);
	            	
	            		break;
	            	
	            	case SQUARE_WAVE:
	            	
						u8g2.drawLine(73, 25, 86, 25);
						u8g2.drawLine(87, 25, 87, 38);
						u8g2.drawLine(88, 38, 104, 38);
						u8g2.drawLine(105, 38, 105, 26);
						u8g2.drawLine(105, 25, 120, 25);
						u8g2.setFont(u8g2_font_profont11_tr);
						u8g2.drawStr(79, 44, "SQUARE");
						myOscillator.setType(Oscillator::square);
	            	
	            		break;
	            	
	            }
	            
	            u8g2.sendBuffer();
	            usleep(200000);
	            break;
		}
    }
}



/*
 --------------------------------------------
| MAIN SETUP FUNCTION CALLED ONCE ON STARTUP |
 --------------------------------------------
*/

bool setup(BelaContext *context, void *userData)
{
	//Initialise each button (digital input) 
	pinMode(context, 0, gInputPin1, INPUT);
	pinMode(context, 0, gInputPin2, INPUT); 
	pinMode(context, 0, gInputPin3, INPUT); 


	//Define frame count dependant on sample rate
	unsigned int numFrames = context->audioSampleRate * gDurationSec;
	gOutputs.resize(context->audioOutChannels);

	//Resize vector to pre-allocate memory for recording
	try {
		for(auto& c : gOutputs){
			c.resize(numFrames);
		}
	} 
	catch (std::exception& e) {
		fprintf(stderr, "Error while allocating memory. Maybe you are asking to record too many frames and/or too many channels\n");
		return false;
	}


	//Initialise vector of touch sensors by scanning i2c addresses

	for(uint8_t addr = 0x20; addr <= 0x29; ++addr)
	{
		Trill::Device device = Trill::probe(1, addr);
		if(Trill::NONE != device && Trill::CRAFT != device)
		{
			gTouchSensors.push_back(new Trill(1, device, addr));
			//gTouchSensors.back()->printDetails();
		}
	}
	


	// Schedule auxiliary task for reading sensor data from the I2C bus
	Bela_runAuxiliaryTask(loop);
	
	
	//Initialise Display
	for(unsigned int n = 0; n < gDisplays.size(); ++n)
		{
			
			U8G2& u8g2 = gDisplays[gActiveTarget].d;
			u8g2.initDisplay();
			u8g2.setPowerSave(0);
			u8g2.clearBuffer();
			u8g2.setFont(u8g2_font_4x6_tf);
			u8g2.setFontRefHeightText();
			u8g2.setFontPosTop();
			u8g2.sendBuffer();
		}
		
	//Create and schedule auxiliary task for display handling	
	oledUpdateTask = Bela_createAuxiliaryTask(oledUpdateLoop, BELA_AUDIO_PRIORITY-1, "OLED Update Task", nullptr);
	Bela_scheduleAuxiliaryTask(oledUpdateTask);
		

    //Initialise all FX and visualizers
	float sampleRate = context->audioSampleRate;
	gInverseSampleRate = 1.0/sampleRate;
	delayEffect.setup(sampleRate);
	highPassFilter.setup(sampleRate, 10000, 0.707);
	lowPassFilter.setup(sampleRate, 15000, 0.707);
	delayLPF.setup(sampleRate, 8000, 0.707);
    myOscillator.setup(sampleRate, Oscillator::sine);
    reverb.setup();
    initParticles(100);

    return true;
}


/*
 ---------------------------------------
| MAIN RENDER FUNCTION FOR AUDIO THREAD |
 ---------------------------------------
*/

void render(BelaContext *context, void *userData)
{

    for(unsigned int n = 0; n < context->audioFrames; n++) {
    	
    	//Handling for digital inputs to detect changes
    	last1ButtonCheck = current1ButtonState;
    	last2ButtonCheck = current2ButtonState;
    	last3ButtonCheck = current3ButtonState;
    	
    	current1ButtonState = digitalRead(context, 0, gInputPin1);
        current2ButtonState = digitalRead(context, 0, gInputPin2);
        current3ButtonState = digitalRead(context, 0, gInputPin3);
        
        if(current1ButtonState && !last1ButtonCheck) gButton1Pressed = true;
        if(current2ButtonState && !last2ButtonCheck) gButton2Pressed = true;
        if(current3ButtonState && !last3ButtonCheck) gButton3Pressed = true;
           
        if (gTouchSensors[1]->compoundTouchSize() > 0.05 | gTouchSensors[0]->compoundTouchSize() > 0.05){
        	//Detects input for touch sensors to show visualizer 
        	viewParamFeed = true;
        }
        
        
        float outputLeft = 0.0f, outputRight = 0.0f;
        float processedOutputLeft = 0.0f, processedOutputRight = 0.0f;


        bool frequencyTouchActive = gTouchSensors[0]->compoundTouchSize() > 0.05;
        bool FXTouchActive = gTouchSensors[1]->compoundTouchSize() > 0.05;

    	
    	
        if (gMainState == PLAY_MODE && frequencyTouchActive) {
        	//Processes touch sensor input and outputs oscillator frequency
        	
            float touchPositionX = 1 - gTouchSensors[0]->compoundTouchLocation();
            float frequency = touchPositionX * 300 + 100; // Maps 0 to 1 into 100Hz to 400Hz
            float oscillatorOutput = myOscillator.process(frequency);
            
            //Applies the signal to left and right channels
            outputLeft = oscillatorOutput;
            outputRight = oscillatorOutput;
            

        } else if (gMainState == VOICE_MODE) {
			//Reads any audio input found in the 3.5mm input
            outputLeft = audioRead(context,n,0);
            outputRight = audioRead(context,n,1);
        }
        
        else {
            //Writes no audio if in main menu
            outputLeft = 0.0;
            outputRight = 0.0;
            
        }
        
        /*
		   -------------------------------------------------------
		  | MAIN SWITCH CASE FOR FX PROCESSING                    |
		  | Audio values from voice/oscillator are processed here |
		   ------------------------------------------------------- 
		*/
        
        switch(gMenuState){
        	
        	case MENU_REVERB:
        		
        		if(FXTouchActive){
        			if(gMenuState == VOICE_MODE){
	        			float wetDryAmount = 0.3 + (1-0.7*gTouchSensors[0]->compoundTouchLocation());
	        			reverb.setWetDryMix(wetDryAmount);
        			}
        			if(gMenuState == PLAY_MODE){
        				//set constant mix value for play mode
        				reverb.setWetDryMix(0.9);
        			}
        			
        			//Map parameters to touch sensors and set values
				    float feedbackAmount = 0.85 + 0.1499*gTouchSensors[1]->compoundTouchLocation();
				    int activeDelays = (int)(4 + 5*gTouchSensors[1]->compoundTouchHorizontalLocation());
        			
        			reverb.setNumberOfActiveDelays(activeDelays);
        			reverb.setFeedbackLevel(feedbackAmount);
        			
        			//Process values and set them to processedOutputLeft/right
        			reverb.process(outputLeft, outputRight, processedOutputLeft, processedOutputRight);
        			
        		}

        		break;
        	
        	case MENU_DELAY:
        		
        		// Determine if there's active input from the delay effect control touch sensor
	        	
		        if (FXTouchActive || delayEffectActive) {
				    // If there's active touch input set the delay and feedback amounts
				    if(FXTouchActive) {
				        lastDelayAmount = gTouchSensors[1]->compoundTouchHorizontalLocation();
				        lastFeedbackAmount = gTouchSensors[1]->compoundTouchLocation();
				    }
				
				    // Set and process the delay and feedback amounts
				    delayEffect.setDelayAmount(lastDelayAmount);
				    delayEffect.setFeedbackAmount(lastFeedbackAmount);
				    
				    delayEffect.process(outputLeft, outputRight, processedOutputLeft, processedOutputRight);
				    
				    processedOutputLeft = delayLPF.process(processedOutputLeft);
			    	processedOutputRight = delayLPF.process(processedOutputRight);
				
				    // Keep the effect active until audio fades
				    delayEffectActive = true;
				} else if (delayEffectActive) {
				    // Continue processing the delay

				    delayEffect.process(outputLeft, outputRight, processedOutputLeft, processedOutputRight);
				    
				    processedOutputLeft = delayLPF.process(processedOutputLeft);
			    	processedOutputRight = delayLPF.process(processedOutputRight);

				}

                break;
        	
        	case MENU_LPF:
        	
        		if (FXTouchActive) {
			        // Map touch positions to cutoff frequency and Q value. Ranges differ depending which mode the system is in
			        //Then process effect to left/right channels
					if(gMainState == PLAY_MODE){

			        	float cutoffFrequency = gTouchSensors[1]->compoundTouchHorizontalLocation() * 2000;
			        	float qValue = 0.3 + (2*gTouchSensors[1]->compoundTouchLocation());
			        	lowPassFilter.setup(context->audioSampleRate, cutoffFrequency, qValue);
			        	processedOutputLeft = lowPassFilter.process(outputLeft);
			        	processedOutputRight = lowPassFilter.process(outputRight); 
					}
					else if(gMainState == VOICE_MODE){

			        	float cutoffFrequency = gTouchSensors[1]->compoundTouchHorizontalLocation() * 5000;
			        	float qValue = 0.4 + (3*gTouchSensors[1]->compoundTouchLocation());
			        	lowPassFilter.setup(context->audioSampleRate, cutoffFrequency, qValue);
			        	processedOutputLeft = lowPassFilter.process(outputLeft);
			        	processedOutputRight = lowPassFilter.process(outputRight); 
					}

        		}
        		
        	break;
        	
        	case MENU_HPF:

        		if (FXTouchActive) {
        		
					if(gMainState == PLAY_MODE){
						
				        float cutoffFrequency = gTouchSensors[1]->compoundTouchHorizontalLocation() * 800;
				        float qValue = 0.4 + (0.6*gTouchSensors[1]->compoundTouchLocation());
				        highPassFilter.setup(context->audioSampleRate, cutoffFrequency, qValue);
				        processedOutputLeft = highPassFilter.process(outputLeft);
			        	processedOutputRight = highPassFilter.process(outputRight);

					}
			
					if(gMainState == VOICE_MODE){

			        	float cutoffFrequency = gTouchSensors[1]->compoundTouchHorizontalLocation() * 2000;
			        	float qValue = 0.4 + (0.3*gTouchSensors[1]->compoundTouchLocation());
			        	highPassFilter.setup(context->audioSampleRate, cutoffFrequency, qValue);
			        	processedOutputLeft = highPassFilter.process(outputLeft);
			        	processedOutputRight = highPassFilter.process(outputRight);

					}
					
        		}
        	
        	break;
        	
        	
        	
        	case MENU_TREMOLO:
        		//LFO implementation adapted from Bela/examples/Audio/tremolo

        		float lfo = sinf(gPhase) * 0.5f;
        		float gPhasePitch = 0;
        		if(gMainState == PLAY_MODE){
        			
        			
	        		if(FXTouchActive){
		        		lfoAmplitude = 0.1 + gTouchSensors[1]->compoundTouchLocation() * 0.9;  // Scale from 0.1 to 1.0

					    // Calculate the LFO frequency based on the vertical touch location
					    float lfoFrequency = max(0.1, gTouchSensors[1]->compoundTouchHorizontalLocation() * 10.0);
					
					    // Update the LFO phase
					    gPhase += 2.0f * (float)M_PI * lfoFrequency * gInverseSampleRate;
					    if(gPhase > 2.0f * (float)M_PI) {
					        gPhase -= 2.0f * (float)M_PI;
					    }

					    float pitchFrequency = 0.5 + gTouchSensors[1]->compoundTouchLocation() * 4.5; // Range from 0.5Hz to 5Hz

					    // Update the pitch shift oscillator phase
					    gPhasePitch += 2.0f * (float)M_PI * pitchFrequency * gInverseSampleRate;
					    if(gPhasePitch > 2.0f * (float)M_PI) {
					        gPhasePitch -= 2.0f * (float)M_PI;
					    }

	        		}
					float lfo = sinf(gPhase) * lfoAmplitude;  // Amplitude varies with horizontal touch
					float pitchShift = sinf(gPhasePitch);

				    // Apply the LFO to the audio output
				    processedOutputLeft = outputLeft * (lfo * (0.5 + 0.5 * pitchShift));
				    processedOutputRight = outputRight * (lfo * (0.5 + 0.5 * pitchShift)); 
        		}
        		
        		else{
        			if(FXTouchActive){
		        		lfoAmplitude = 0.1 + gTouchSensors[1]->compoundTouchLocation() * 0.9;  // Scale from 0.1 to 1.0
					    float lfoFrequency = max(0.1, gTouchSensors[1]->compoundTouchHorizontalLocation() * 10.0);
					
					    gPhase += 2.0f * (float)M_PI * lfoFrequency * gInverseSampleRate;
					    if(gPhase > 2.0f * (float)M_PI) {
					        gPhase -= 2.0f * (float)M_PI;
					    }
					    
					    
					    float pitchFrequency = 0.5 + gTouchSensors[1]->compoundTouchLocation() * 4.5; // Range from 0.5Hz to 5Hz

					    gPhasePitch += 2.0f * (float)M_PI * pitchFrequency * gInverseSampleRate;
					    if(gPhasePitch > 2.0f * (float)M_PI) {
					        gPhasePitch -= 2.0f * (float)M_PI;
					    }
					    
					    
					    

	        		}
					float lfo = sinf(gPhase) * lfoAmplitude;  // Now amplitude varies with horizontal touch
					float pitchShift = sinf(gPhasePitch);

				    // Apply the LFO to the audio output
				    processedOutputLeft = outputLeft * (lfo * (0.5 + 0.5 * pitchShift));
				    processedOutputRight = outputRight * (lfo * (0.5 + 0.5 * pitchShift)); 
        		}
        		
        	break;
        	
        }
        
        
        //Recording functoinality of processed output handled here, approach adapted from Bela/examples/Audio/record-audio

        if(recordMode){

	        if(curReccording){
	        	//Writes each audio frame for left and right channels to gOutputs
	        	gOutputs[0][gWrittenFrames] = processedOutputLeft;
	        	gOutputs[1][gWrittenFrames] = processedOutputRight;
				audioWrite(context, n, 0, processedOutputLeft);
	        	audioWrite(context, n, 1, processedOutputRight);
	        	
	        	++gWrittenFrames;
	        	
	        	if(gWrittenFrames >= gOutputs[0].size()) {
					// When the buffer is filled, cancel the recording and trigger the Write function

					curReccording = false;
					recComplete = true;

					return;
					
				}
	        }
	        
	        
        }
        
        //Writing audio output from FX switch case
        audioWrite(context, n, 0, processedOutputLeft);
        audioWrite(context, n, 1, processedOutputRight);
        
    	
    	
    	
	}

}


/*
 ------------------------------------------------------
| MAIN CLEANUP FUNCTION CALLED WHEN BOARD IS SHUT DOWN |
 ------------------------------------------------------
*/

void cleanup(BelaContext *context, void *userData)
{
	delayEffect.cleanup();
	//Clear any memory and display goodbye message
	U8G2& u8g2 = gDisplays[gActiveTarget].d;
	u8g2.clearBuffer();
	u8g2.drawStr(0,0,"GOODBYE");
	u8g2.sendBuffer();

	usleep(1000000);
	
	u8g2.clearBuffer();
	u8g2.sendBuffer();

}
