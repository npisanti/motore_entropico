#pragma once

#define NUMSYNTHS 4

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxPDSP.h"
#include "ofxGui.h"
#include "FMMono.h"
#include "ModalTable.h"
#include "StereoDelay.h"
#include "synth/WaveSynth.h"
#include "effect/Chorus.h"
#include "effect/Filter.h"
#include "meter/RMS.h"
#include "dynamics/Brickwall.h"
#include "synth/KarplusStrong.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
        
        void drawMeter(float value, float min, float max, int x, int y, int w, int h);
        
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
        
        void oscMapping();
        
        pdsp::Engine        engine;   
        pdsp::osc::Input    osc;
          
        pdsp::ParameterGain fader;

        std::vector<np::synth::FMMono>  synths;

        pdsp::ParameterGain reverbGain;
        pdsp::BasiVerb    reverb;
        npl::effect::StereoDelay delays;

        np::tuning::ModalTable table;
        
        ofxPanel synthsGUI;
        ofxPanel fxGUI;
        ofxPanel tuningGUI;

        
        np::synth::WaveSynth wtsynth;
        np::effect::Filter filter;
        np::effect::Chorus chorus;
        std::vector<float> dtriggers;
        
        void initWaveTable( pdsp::WaveTable & wt );
        
        np::dynamics::Brickwall limiter;
        pdsp::LowCut revcut;
        pdsp::LowCut delaycut;
        pdsp::LowCut dtcut;
        

        np::synth::KarplusStrong ksynth;
        std::vector<float> ktriggers;
    
        bool bDrawGui;
        
};
