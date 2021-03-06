
// FMMono.cpp
// Nicola Pisanti, MIT License, 2018

#include "FMMono.h"

void np::synth::FMMono::patch(){

    parameters.setName( "fm monosynth" );
    
    parameters.add( gain.set("gain", -12, -48, 12) );

    parameters.add( env_release_ctrl.set("env release", 200, 5, 2000));   
    parameters.add( drift.set("pitch drift", 0.0f, 0.0f, 1.0f) );    

    // ---------------- PATCHING ------------------------
    addModuleInput("trig", voiceTrigger);
    addModuleInput("pitch", pitchSlew );
    addModuleInput("decay", decayControl );
    addModuleInput("ratio", modulator.in_ratio() );
    addModuleInput("fm_amount", fmModAmount.in_mod() );
    addModuleInput("fb_amount", selfModAmount.in_mod() );
    addModuleInput("other", otherControl.in_signal() );
    addModuleInput("other_amount", otherControl.in_mod() );
    addModuleOutput( "gain", gain );
    addModuleOutput( "signal", voiceAmp );

    // SIGNAL PATH -----------------------------
    modulator >> fmAmp >> carrier.in_fm() >> voiceAmp >> gain;
          otherControl >> carrier.in_fm();
    
    0.2f    >> phazorFree;
    0.05f  >> randomSlew.in_freq();
                                         drift >> driftAmt.in_mod();        
    phazorFree.out_trig() >> rnd >> randomSlew >> driftAmt >> pitchNode;
    pitchSlew >> pitchNode;
    
    // MODULATIONS AND CONTROL -----------------
   
    attackControl.set( 0.0f );
    slewControl.set( 20000.0f );
    slewControl >> pitchSlew.in_freq();

    pitchNode >> carrier.in_pitch();
    pitchNode >> modulator.in_pitch();

    voiceTrigger >> ampEnv >> voiceAmp.in_mod();
    voiceTrigger >> modEnv;

    modEnv >> fmModAmount   >> fmAmp.in_mod();
    modEnv >> selfModAmount >> carrier.in_fb();

    attackControl  >> ampEnv.in_attack();
    decayControl   >> ampEnv.in_decay();
    0.45f >> ampEnv.in_sustain();
    env_release_ctrl >> ampEnv.in_release();

    attackControl  >> modEnv.in_attack();
    decayControl   >> modEnv.in_decay();
    0.0f >> modEnv.in_sustain();
    env_release_ctrl >> modEnv.in_release();

    lastTrigger = 0.0f;    
    lastOtherTrigger = 0.0f;

}

float np::synth::FMMono::meter_env() const{
    return ampEnv.meter_output();
}

float np::synth::FMMono::meter_pitch() const{
    return carrier.meter_pitch();
}

ofParameterGroup & np::synth::FMMono::label (std::string name ){
    parameters.setName( name );
    return parameters;
}
