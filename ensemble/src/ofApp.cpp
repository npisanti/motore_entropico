
#include "ofApp.h"

#define FRAGW 320
#define CAMW 240
#define CAMH 240

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetWindowTitle("|::|");
    engine.score.setTempo( 120.0f); // the delay times are clocked

    bDrawGui = true;

    osc.setVerbose( true );
    osc.openPort( 4444 );

    synths.resize( NUMSYNTHS );

    table.setup( 6, "modal table" );
    
    for( size_t i=0; i<synths.size(); ++i ){
        synths[i].out("gain") >> delaycut.ch(0) >> delays.ch(0);
        synths[i].out("gain") >> delaycut.ch(1) >> delays.ch(1);   
        synths[i].out("gain") >> reverbGain; 
    }
    
    reverbGain >> revcut >> reverb; 
    
    for( int i=0; i<NUMSYNTHS; ++i ){
        synths[i].out("gain") * pdsp::panL(pdsp::spread(i, NUMSYNTHS, 0.5f) ) >> limiter.ch(0);
        synths[i].out("gain") * pdsp::panR(pdsp::spread(i, NUMSYNTHS, 0.5f) ) >> limiter.ch(1);  
    }

    synths[1].out("signal") >> synths[0].in("other");
    synths[2].out("signal") >> synths[1].in("other");
    synths[3].out("signal") >> synths[2].in("other");
    synths[0].out("signal") >> synths[3].in("other");

    delays.ch(0) >> reverb;
    delays.ch(1) >> reverb;
    
    reverb.ch(0) >> limiter.ch(0);
    reverb.ch(1) >> limiter.ch(1);
 
    delays.ch(0) >> limiter.ch(0);
    delays.ch(1) >> limiter.ch(1);
   
    limiter.ch(0) >> engine.audio_out(0);
    limiter.ch(1) >> engine.audio_out(1);


    initWaveTable( wtsynth.wavetable );
    wtsynth.setup( 8 );
    dtriggers.resize( wtsynth.voices.size() );
    for(size_t i=0; i<wtsynth.voices.size(); ++i){
        dtriggers[i] = 0.0f;
    }
    
    wtsynth.ch(0) >> dtcut >> filter >> reverbGain;
                              filter >> chorus.ch(0) >> limiter.ch(0);
                              filter >> chorus.ch(1) >> limiter.ch(1);

    100.0f >> dtcut.in_freq();
    100.0f >> revcut.in_freq();
    100.0f >> delaycut.in_freq();

    ksynth.setup( 4, 0.75f );
    ktriggers.resize( ksynth.size() );
    for(size_t i=0; i<ksynth.size(); ++i){
        ktriggers[i] = 0.0f;
    }
    ksynth.ch( 0 ) >> limiter.ch( 0 );
    ksynth.ch( 1 ) >> limiter.ch( 1 );

    // OSC mapping -----------------------------
    osc.linkTempo( "/orca/bpm" );
    oscMapping();
    
    // graphic setup---------------------------
    ofSetVerticalSync(true);
    ofDisableAntiAliasing();
    ofBackground(0);
    ofSetColor(ofColor(0,100,100));
    ofNoFill();
    ofSetLineWidth(1.0f);
    ofSetFrameRate(24);

    // GUI -----------------------------------

    synthsGUI.setup("SYNTHS", "synths.xml", 220, 10);
        synthsGUI.add( synths[0].label("fm synth A") );
        synthsGUI.add( synths[1].label("fm synth B") );
        synthsGUI.add( synths[2].label("fm synth C") );
        synthsGUI.add( synths[3].label("fm synth D") );
        synthsGUI.add( wtsynth.parameters );
        synthsGUI.add( ksynth.parameters );
        synthsGUI.add( filter.parameters );
        synthsGUI.add( chorus.parameters );
        synthsGUI.add( limiter.parameters );
    synthsGUI.loadFromFile("synths.xml");
    
    fxGUI.setup("FX", "fx.xml", 10, 320);
        fxGUI.add( delays.parameters );
        fxGUI.add( reverbGain.set("reverb gain", -12, -36, 0 ) );
    fxGUI.loadFromFile( "fx.xml" );
    
    tuningGUI.setup("TUNING", "tuning.xml", 10, 10 );
    tuningGUI.add( table.parameters );
    tuningGUI.loadFromFile( "tuning.xml" );
        

    // audio setup----------------------------
    engine.sequencer.play();
    
    engine.addOscInput ( osc );
    
    engine.listDevices();
    engine.setDeviceID(0); 
    
    engine.setup( 44100, 512, 3);     
}

        
//--------------------------------------------------------------
void ofApp::initWaveTable( pdsp::WaveTable & wt ){
    
    wt.setup( 512, 128 ); // 512 samples, 128 max partials
   
    wt.addSineWave();
    wt.addTriangleWave( pdsp::highestPartial(60.0f));    
    wt.addSquareWave( pdsp::highestPartial(60.0f) ); 
    wt.addSawWave( pdsp::highestPartial(60.0f) ); 

    wt.addAdditiveWave ( { 1.0f, 1.0f, 1.0f, 1.0f } ); 
    wt.addAdditiveWave ({ 1.0f, 0.0f, -1.0f, 0.5f, 0.5f, 1.0f, -1.0f, 0.5f, 0.5f, 1.0f, -1.0f, 0.5f, 0.5f, 1.0f, -1.0f, 0.5f }, true ); 

}


//--------------------------------------------------------------
void ofApp::oscMapping(){

    for( int index = 0; index< NUMSYNTHS; ++index ){
        std::cout<< "MAPPING SYNTH "<<index<<"\n";
        osc.out_trig("/x", index) >> synths[index].in("trig");  
        osc.parser("/x", index) = [&, index]( float value ) noexcept {
            if( value==synths[index].lastTrigger ){ return pdsp::osc::Ignore;
            }else{ synths[index].lastTrigger = value; return value;  }
        };
        
        osc.out_value("/x", index+NUMSYNTHS+1) >> synths[index].in("pitch");
        osc.parser("/x", index+NUMSYNTHS+1) = [&]( float value ) noexcept {
            int i = value;
            float p = table.pitches[i%table.degrees];
            int o = i / table.degrees;
            p += o*12.0f;
            return p;  
        };        
        
        osc.out_value("/c", index) * 0.125f >> synths[index].in("other_amount");

        osc.out_value("/r", index) >> synths[index].in("ratio");
        osc.parser("/r", index) = [&, index]( float value ) noexcept {
            if( value==0.0f ){ 
                return 0.5f;
            }else if( value==17.0f ){ 
                return 0.5f;
            }else if( value==19.0f ){ 
                return 0.25f;
            }else if( value==20.0f ){ 
                return 0.125f;
            }else if( value>20.0f ){
                 return 16.0f;
            }else{
                return value;
            }
        };

        osc.out_value("/o", index) * 12.0f >> synths[index].in("pitch");
            
        osc.out_value("/s", index) * 0.03f >> synths[index].in("fb_amount");        

        osc.out_value("/f", index) * 0.1f >> synths[index].in("fm_amount");

        osc.out_value("/d", index) >> synths[index].in("decay");
        osc.parser("/d", index) = [&]( float value ) noexcept {
            value *= 0.112;
            value = (value<1.0) ? value : 1.0;
            value = value * value * 4000.0f;
            return value;  
        };      
        
        osc.parser("/a", index) = [&, index]( float value ) noexcept {
            int slew = value;
 
            switch( slew ){ 
                case 0: synths[index].slewControl.set(20000.0f); break;
                case 1: synths[index].slewControl.set( 150.0f); break;
                case 2: synths[index].slewControl.set( 60.0f); break;
                case 3: synths[index].slewControl.set( 50.0f); break;
                case 4: synths[index].slewControl.set( 40.0f); break;
                case 5: synths[index].slewControl.set( 35.0f); break;
                case 6: synths[index].slewControl.set( 30.0f); break;
                case 7: synths[index].slewControl.set( 25.0f); break;
                case 8: default: synths[index].slewControl.set( 20.0f); break;
            }
            
            value *= 0.112;
            value = (value<1.0) ? value : 1.0;
            value = value * value * 500.0f;
            value += 4;
            synths[index].attackControl.set( value );
            return pdsp::osc::Ignore;
        };      
        
        
        osc.out_trig("/k", index) >> ksynth.in_trig( index );  
        osc.parser("/k", index) = [&, index]( float value ) noexcept {
            if( value==ktriggers[index] ){ return pdsp::osc::Ignore;
            }else{ ktriggers[index] = value; return value;  }
        };
        
        osc.out_value("/k", index+NUMSYNTHS+1) >> ksynth.in_pitch( index );
        osc.parser("/k", index+NUMSYNTHS+1) = [&]( float value ) noexcept {
            int i = value;
            float p = table.pitches[i%table.degrees];
            int o = i / table.degrees;
            p += o*12.0f;
            return p;  
        };       
        
        osc.out_value("/l", 0 ) * 12.0f >> ksynth.in_pitch( index );
        osc.out_value("/l", 1 )  >> ksynth.voices[index].in("pluck_decay");
        osc.out_value("/l", 2 )  >> ksynth.voices[index].in("fb");
    }
    
    osc.parser("/l", 1) = [&]( float value ) noexcept {
        value *= 0.112;
        value = (value<1.0) ? value : 1.0;
        value = value * value;
        value = 1 + value * 499;
        return value;  
    };          
    
    osc.parser("/l", 2) = [&]( float value ) noexcept {
        value *= 0.112;
        value = (value<1.0) ? value : 1.0;
        value = 1.0f - value;
        value = value * value * value;
        value = 1.0f - value;
        value = value * 0.25f;
        return value;  
    };          
    

    for( size_t index = 0; index<wtsynth.voices.size(); ++index ){
        
        osc.out_trig("/t", index) >> wtsynth.voices[index].in("trig");  
        osc.parser("/t", index) = [&, index]( float value ) noexcept {
            if( value==dtriggers[index] ){ return pdsp::osc::Ignore;
            }else{ dtriggers[index]  = value; return value;  }
        };        
        
        osc.out_value("/p", index) >> wtsynth.voices[index].in("pitch");  
        osc.parser("/p", index) = [&]( float value ) noexcept {
            int i = value;
            float p = table.pitches[i%table.degrees];
            int o = i / table.degrees;
            p += o*12.0f;
            return p;  
        };        
        osc.out_value("/p", index).enableSmoothing( 65.0f );
        
        osc.out_value("/q", index) * 12.0f >> wtsynth.voices[index].in("pitch");  
        // FIX pdsp::WTOscillator and then remove this 
        osc.parser("/q", index) = [&]( float value ) noexcept {
            if( value > 4.0f ) value = 4.0f;
            return value;  
        };  
        
        osc.out_value("/v", 0 ) >> wtsynth.voices[index].in("table");
        osc.out_value("/v", 1 ) * (1.0f/32.0f) >> wtsynth.voices[index].in("table");
    }
    
    osc.out_value("/v", 1 ).enableSmoothing( 30.0f );
    osc.out_value("/v", 2 ) * 15.0 >> filter.in_cutoff();
    osc.out_value("/v", 2 ).enableSmoothing( 5000.0f ); 
    osc.out_value("/v", 3 ) * 12.0 >> filter.in_cutoff();
    osc.out_value("/v", 3 ).enableSmoothing( 20.0f ); 
 
}

//--------------------------------------------------------------
void ofApp::update(){
    
}

//--------------------------------------------------------------
void ofApp::draw(){    
    if(bDrawGui){
        tuningGUI.draw();
        synthsGUI.draw();
        fxGUI.draw();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch( key ){
        case 'g': bDrawGui = !bDrawGui; break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
