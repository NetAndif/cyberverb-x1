#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cmath>

void CyberverbX1AudioProcessor::SpaceDspUnit::prepare (const juce::dsp::ProcessSpec& spec)
{
    dlyLineL.prepare (spec);
    dlyLineR.prepare (spec);
    preDlyL.prepare (spec);
    preDlyR.prepare (spec);

    dlyToneFilterL.prepare (spec);
    dlyToneFilterR.prepare (spec);
    dlyToneFilterL.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    dlyToneFilterR.setType (juce::dsp::StateVariableTPTFilterType::lowpass);

    reverb.prepare (spec);
    reset();
}

void CyberverbX1AudioProcessor::SpaceDspUnit::reset()
{
    dlyLineL.reset();
    dlyLineR.reset();
    preDlyL.reset();
    preDlyR.reset();
    dlyToneFilterL.reset();
    dlyToneFilterR.reset();
    reverb.reset();
    fbStateL = 0.0f;
    fbStateR = 0.0f;
    modPhase = 0.0f;
}

CyberverbX1AudioProcessor::CyberverbX1AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#else
    : apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    for (int k = 0; k < numAssignKnobs; ++k)
    {
        const auto idx = static_cast<size_t> (k);
        assignKnobMidiCC[idx].store (-1, std::memory_order_relaxed);
        assignPickupLocked[idx].store (false, std::memory_order_relaxed);
        lastMidiCCNorm[idx].store (-1.0f, std::memory_order_relaxed);
        assignKnobCCMode[idx].store (static_cast<int> (MidiCCMode::Absolute), std::memory_order_relaxed);
    }

    cacheParameterPointers();
    ensureDefaultPresetExists();
    installFactoryPresets (false);
}

CyberverbX1AudioProcessor::~CyberverbX1AudioProcessor() {}

void CyberverbX1AudioProcessor::cacheParameterPointers()
{
    params.bypass      = apvts.getRawParameterValue ("sys_bypass");
    params.freeze      = apvts.getRawParameterValue ("sys_freeze");
    params.mixDry      = apvts.getRawParameterValue ("mix_dry");
    params.mixWet      = apvts.getRawParameterValue ("mix_wet");
    params.spaceALevel = apvts.getRawParameterValue ("space_a_lvl");
    params.spaceMorph  = apvts.getRawParameterValue ("space_morph");
    params.spaceBLevel = apvts.getRawParameterValue ("space_b_lvl");
    params.outGainDb   = apvts.getRawParameterValue ("out_gain");
    params.outWidth    = apvts.getRawParameterValue ("out_width");

    params.sysMode      = apvts.getRawParameterValue ("sys_mode");
    params.sysSplitFreq = apvts.getRawParameterValue ("sys_split_freq");
    params.sysDrive     = apvts.getRawParameterValue ("sys_drive");
    params.sysCrossFeed = apvts.getRawParameterValue ("sys_cross_feed");
    params.duckAmt      = apvts.getRawParameterValue ("duck_amt");
    params.duckRel      = apvts.getRawParameterValue ("duck_rel");
    params.gateThr      = apvts.getRawParameterValue ("gate_thr");
    params.gateRel      = apvts.getRawParameterValue ("gate_rel");

    params.lfo1Rate  = apvts.getRawParameterValue ("lfo1_rate");
    params.lfo1Shape = apvts.getRawParameterValue ("lfo1_shape");
    params.lfo2Rate  = apvts.getRawParameterValue ("lfo2_rate");
    params.lfo2Shape = apvts.getRawParameterValue ("lfo2_shape");
    params.envSens   = apvts.getRawParameterValue ("env_sens");
    params.envAtk    = apvts.getRawParameterValue ("env_atk");
    params.envRel    = apvts.getRawParameterValue ("env_rel");
    params.envBias   = apvts.getRawParameterValue ("env_bias");

    params.eqLowFreq  = apvts.getRawParameterValue ("eq_low_freq");
    params.eqLowGain  = apvts.getRawParameterValue ("eq_low_gain");
    params.eqMid1Freq = apvts.getRawParameterValue ("eq_mid1_freq");
    params.eqMid1Gain = apvts.getRawParameterValue ("eq_mid1_gain");
    params.eqMid2Freq = apvts.getRawParameterValue ("eq_mid2_freq");
    params.eqMid2Gain = apvts.getRawParameterValue ("eq_mid2_gain");
    params.eqHighFreq = apvts.getRawParameterValue ("eq_high_freq");
    params.eqHighGain = apvts.getRawParameterValue ("eq_high_gain");

    for (size_t s = 0; s < 2; ++s)
    {
        const auto p = (s == 0) ? "a_" : "b_";
        auto& sp = params.spaces[s];

        sp.on        = apvts.getRawParameterValue (juce::String (p) + "on");
        sp.dlyTime   = apvts.getRawParameterValue (juce::String (p) + "dly_time");
        sp.dlySync   = apvts.getRawParameterValue (juce::String (p) + "dly_sync");
        sp.dlyFeed   = apvts.getRawParameterValue (juce::String (p) + "dly_feed");
        sp.dlyMode   = apvts.getRawParameterValue (juce::String (p) + "dly_mode");
        sp.dlyTone   = apvts.getRawParameterValue (juce::String (p) + "dly_tone");
        sp.dlyRevMix = apvts.getRawParameterValue (juce::String (p) + "dly_rev_mix");

        sp.revAlgo    = apvts.getRawParameterValue (juce::String (p) + "rev_algo");
        sp.revDecay   = apvts.getRawParameterValue (juce::String (p) + "rev_decay");
        sp.revSize    = apvts.getRawParameterValue (juce::String (p) + "rev_size");
        sp.revDamp    = apvts.getRawParameterValue (juce::String (p) + "rev_damp");
        sp.revPreDly  = apvts.getRawParameterValue (juce::String (p) + "rev_predly");
        sp.revDiffuse = apvts.getRawParameterValue (juce::String (p) + "rev_diffuse");
        sp.revMod     = apvts.getRawParameterValue (juce::String (p) + "rev_mod");
    }

    for (size_t m = 0; m < 4; ++m)
    {
        const auto mp = "mod" + juce::String (static_cast<int> (m + 1)) + "_";
        params.modSlots[m].src    = apvts.getRawParameterValue (mp + "src");
        params.modSlots[m].dst    = apvts.getRawParameterValue (mp + "dst");
        params.modSlots[m].amount = apvts.getRawParameterValue (mp + "amt");
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout CyberverbX1AudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Global & Master
    p.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { "sys_bypass", 1 }, "Bypass", false));
    p.push_back (std::make_unique<juce::AudioParameterBool>  (juce::ParameterID { "sys_freeze", 1 }, "Freeze Tails", false));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix_dry", 1 }, "Dry Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix_wet", 1 }, "Wet Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.45f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "space_a_lvl", 1 }, "Space A Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.85f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "space_morph", 1 }, "Space Morph",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.50f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "space_b_lvl", 1 }, "Space B Level",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.85f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out_gain", 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "out_width", 1 }, "Stereo Width",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f));

    // Routing & Dynamics
    const juce::StringArray sysModes = { "Parallel", "Serial A > B", "Serial B > A", "Mid / Side", "Band Split" };
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "sys_mode", 1 }, "System Routing", sysModes, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sys_split_freq", 1 }, "Split Frequency",
        juce::NormalisableRange<float> (60.0f, 8000.0f, 1.0f, 0.30f), 450.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sys_drive", 1 }, "Input Drive",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sys_cross_feed", 1 }, "Cross Feedback",
        juce::NormalisableRange<float> (0.0f, 0.85f, 0.001f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "duck_amt", 1 }, "Ducking Amount",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "duck_rel", 1 }, "Ducking Release",
        juce::NormalisableRange<float> (0.02f, 2.0f, 0.001f, 0.4f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gate_thr", 1 }, "Gate Threshold",
        juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -60.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "gate_rel", 1 }, "Gate Release",
        juce::NormalisableRange<float> (0.01f, 1.0f, 0.001f, 0.4f), 0.15f));

    // Space A & B Hybrid Engines (Delay + Reverb)
    const juce::StringArray dlyModes   = { "Stereo", "Ping-Pong", "Tape Echo", "Dual Tap" };
    const juce::StringArray dlySyncs   = { "1/16", "1/8 Dot", "1/8", "1/4", "1/4 Dot", "1/2", "1/1" };
    const juce::StringArray revAlgosA  = { "Plate", "Room", "Hall", "Chamber", "Vintage Spring" };
    const juce::StringArray revAlgosB  = { "Shimmer Cloud", "Cyber Hall", "Granular Mist", "Dark Void", "Reverse Bloom" };

    for (int s = 0; s < 2; ++s)
    {
        const juce::String pre   = (s == 0) ? "a_" : "b_";
        const juce::String label = (s == 0) ? "Space A " : "Space B ";

        p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { pre + "on", 1 }, label + "On", true));

        // Delay
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "dly_time", 1 }, label + "Delay Time",
            juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.35f), (s == 0) ? 240.0f : 375.0f));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pre + "dly_sync", 1 }, label + "Delay Sync",
            dlySyncs, (s == 0) ? 2 : 1));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "dly_feed", 1 }, label + "Delay Feedback",
            juce::NormalisableRange<float> (0.0f, 1.05f, 0.001f), (s == 0) ? 0.35f : 0.45f));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pre + "dly_mode", 1 }, label + "Delay Mode",
            dlyModes, (s == 0) ? 0 : 1));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "dly_tone", 1 }, label + "Delay Tone",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), (s == 0) ? 0.70f : 0.45f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "dly_rev_mix", 1 }, label + "Delay > Reverb Mix",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.50f));

        // Reverb
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { pre + "rev_algo", 1 }, label + "Reverb Algo",
            (s == 0) ? revAlgosA : revAlgosB, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_decay", 1 }, label + "Decay",
            juce::NormalisableRange<float> (0.1f, 25.0f, 0.01f, 0.35f), (s == 0) ? 1.8f : 4.5f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_size", 1 }, label + "Size",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), (s == 0) ? 0.52f : 0.82f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_damp", 1 }, label + "Damping",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), (s == 0) ? 0.45f : 0.30f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_predly", 1 }, label + "Pre-Delay",
            juce::NormalisableRange<float> (0.0f, 500.0f, 0.5f, 0.45f), (s == 0) ? 18.0f : 45.0f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_diffuse", 1 }, label + "Diffusion / Shimmer",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), (s == 0) ? 0.75f : 0.60f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { pre + "rev_mod", 1 }, label + "Modulation",
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), (s == 0) ? 0.25f : 0.55f));
    }

    // Modulation & Envelope Follower
    const juce::StringArray lfoShapes = { "Sine", "Triangle", "Saw Up", "Square", "Sample & Hold" };
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lfo1_rate", 1 }, "LFO 1 Rate",
        juce::NormalisableRange<float> (0.05f, 15.0f, 0.01f, 0.35f), 0.45f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo1_shape", 1 }, "LFO 1 Shape", lfoShapes, 0));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "lfo2_rate", 1 }, "LFO 2 Rate",
        juce::NormalisableRange<float> (0.05f, 15.0f, 0.01f, 0.35f), 1.80f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lfo2_shape", 1 }, "LFO 2 Shape", lfoShapes, 1));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "env_sens", 1 }, "Env Sensitivity",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "env_atk", 1 }, "Env Attack",
        juce::NormalisableRange<float> (0.001f, 0.5f, 0.001f, 0.4f), 0.015f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "env_rel", 1 }, "Env Release",
        juce::NormalisableRange<float> (0.01f, 2.0f, 0.001f, 0.4f), 0.22f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "env_bias", 1 }, "Env Bias",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

    // 4-Band Wet EQ
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_low_freq", 1 }, "EQ Low Freq",
        juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.4f), 110.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_low_gain", 1 }, "EQ Low Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), -2.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_mid1_freq", 1 }, "EQ Mid 1 Freq",
        juce::NormalisableRange<float> (200.0f, 2500.0f, 1.0f, 0.4f), 650.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_mid1_gain", 1 }, "EQ Mid 1 Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_mid2_freq", 1 }, "EQ Mid 2 Freq",
        juce::NormalisableRange<float> (800.0f, 8000.0f, 1.0f, 0.4f), 2800.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_mid2_gain", 1 }, "EQ Mid 2 Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_high_freq", 1 }, "EQ High Freq",
        juce::NormalisableRange<float> (2000.0f, 18000.0f, 1.0f, 0.4f), 7500.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "eq_high_gain", 1 }, "EQ High Gain",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 1.5f));

    // Mod Matrix (4 Slots)
    const juce::StringArray modSrc = {
        "Off", "LFO 1", "LFO 2", "Env Follower", "Space Morph"
    };
    const juce::StringArray modDst = {
        "Off", "Space Morph", "Stereo Width",
        "A: Delay Time", "A: Delay Feed", "A: Decay", "A: Size",
        "B: Delay Time", "B: Delay Feed", "B: Decay", "B: Size"
    };

    for (int m = 0; m < 4; ++m)
    {
        const auto mp = "mod" + juce::String (m + 1) + "_";
        const auto ml = "Mod " + juce::String (m + 1) + " ";
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { mp + "src", 1 }, ml + "Source", modSrc, 0));
        p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { mp + "dst", 1 }, ml + "Destination", modDst, 0));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { mp + "amt", 1 }, ml + "Amount",
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));
    }

    return { p.begin(), p.end() };
}

void CyberverbX1AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRateHz.store (sampleRate);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels      = 2;

    for (auto& sp : dspSpaces)
        sp.prepare (spec);

    crossoverLow.prepare (spec);
    crossoverHigh.prepare (spec);
    crossoverLow.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
    crossoverHigh.setType (juce::dsp::LinkwitzRileyFilterType::highpass);

    wetEqLow.prepare (spec);
    wetEqMid1.prepare (spec);
    wetEqMid2.prepare (spec);
    wetEqHigh.prepare (spec);
    updateWetEqFilters();
}

void CyberverbX1AudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CyberverbX1AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
#endif

void CyberverbX1AudioProcessor::updateWetEqFilters()
{
    const double sr = juce::jmax (8000.0, currentSampleRateHz.load());
    *wetEqLow.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sr, params.eqLowFreq->load(),  0.707f, juce::Decibels::decibelsToGain (params.eqLowGain->load()));
    *wetEqMid1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, params.eqMid1Freq->load(), 1.1f,   juce::Decibels::decibelsToGain (params.eqMid1Gain->load()));
    *wetEqMid2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sr, params.eqMid2Freq->load(), 1.1f,   juce::Decibels::decibelsToGain (params.eqMid2Gain->load()));
    *wetEqHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, params.eqHighFreq->load(), 0.707f, juce::Decibels::decibelsToGain (params.eqHighGain->load()));
}

void CyberverbX1AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numChannels < 2 || numSamples <= 0)
        return;

    if (flushRequested.exchange (false))
    {
        for (auto& sp : dspSpaces)
            sp.reset();
        crossFbAtoB = crossFbBtoA = 0.0f;
    }

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                currentBpm.store (static_cast<float> (*bpm));

    // MIDI Learn & Relative/Absolute CC-Steuerung für die 8 rechten Performance-Regler
    const char* assignParamIDs[numAssignKnobs] = {
        "a_rev_decay", "b_rev_decay",
        "a_rev_size",  "b_rev_size",
        "a_rev_damp",  "b_rev_damp",
        "a_dly_feed",  "b_dly_feed"
    };

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (! msg.isController())
            continue;

        midiActivityFlag.store (true);
        const int ccNum    = msg.getControllerNumber();
        const int rawCCVal = msg.getControllerValue();
        const float norm   = static_cast<float> (rawCCVal) / 127.0f;

        const int learnSlot = midiLearnTargetSlot.exchange (-1, std::memory_order_acq_rel);
        if (learnSlot >= 0 && learnSlot < numAssignKnobs)
        {
            const auto slotIdx = static_cast<size_t> (learnSlot);
            assignKnobMidiCC[slotIdx].store (ccNum, std::memory_order_release);
            assignPickupLocked[slotIdx].store (false, std::memory_order_release);
            lastMidiCCNorm[slotIdx].store (-1.0f, std::memory_order_release);
            lastLearnedAssignSlot.store (learnSlot, std::memory_order_release);
        }

        const bool usePickup = midiPickupEnabled.load (std::memory_order_acquire);
        for (int k = 0; k < numAssignKnobs; ++k)
        {
            const auto idx = static_cast<size_t> (k);
            if (assignKnobMidiCC[idx].load (std::memory_order_acquire) != ccNum)
                continue;

            auto* param = apvts.getParameter (assignParamIDs[k]);
            if (param == nullptr)
                continue;

            const auto ccMode = static_cast<MidiCCMode> (assignKnobCCMode[idx].load (std::memory_order_acquire));
            const float curVal = param->getValue();

            if (ccMode != MidiCCMode::Absolute)
            {
                int delta = 0;
                if (ccMode == MidiCCMode::RelativeTwosComp)
                    delta = (rawCCVal <= 63) ? rawCCVal : (rawCCVal >= 65 ? rawCCVal - 128 : 0);
                else
                    delta = (rawCCVal >= 65) ? (rawCCVal - 64) : (rawCCVal <= 63 ? rawCCVal - 64 : 0);

                if (delta != 0)
                    param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, curVal + static_cast<float> (delta) / 127.0f));
                continue;
            }

            const float prevMidi = lastMidiCCNorm[idx].exchange (norm, std::memory_order_acq_rel);
            if (! usePickup)
            {
                assignPickupLocked[idx].store (true, std::memory_order_release);
                param->setValueNotifyingHost (norm);
                continue;
            }

            bool locked = assignPickupLocked[idx].load (std::memory_order_acquire);
            if (locked && prevMidi >= 0.0f && std::abs (curVal - prevMidi) > 0.025f)
            {
                locked = false;
                assignPickupLocked[idx].store (false, std::memory_order_release);
            }

            if (! locked)
            {
                if (std::abs (norm - curVal) <= 0.035f
                    || (prevMidi >= 0.0f && ((prevMidi < curVal && norm >= curVal) || (prevMidi > curVal && norm <= curVal))))
                {
                    locked = true;
                    assignPickupLocked[idx].store (true, std::memory_order_release);
                }
            }

            if (locked)
                param->setValueNotifyingHost (norm);
        }
    }

    // Input Peak L & R messen
    const float inPeakL = buffer.getMagnitude (0, 0, numSamples);
    const float inPeakR = buffer.getMagnitude (1, 0, numSamples);
    const float inPeak  = juce::jmax (inPeakL, inPeakR);

    float prevInL = inputPeakL.load (std::memory_order_relaxed);
    while (inPeakL > prevInL && ! inputPeakL.compare_exchange_weak (prevInL, inPeakL, std::memory_order_relaxed)) {}

    float prevInR = inputPeakR.load (std::memory_order_relaxed);
    while (inPeakR > prevInR && ! inputPeakR.compare_exchange_weak (prevInR, inPeakR, std::memory_order_relaxed)) {}

    const bool isBypassed = (params.bypass->load() > 0.5f);
    const bool tailsOn    = tailsEnabled.load (std::memory_order_acquire);
    const bool muteInput  = muteInEnabled.load (std::memory_order_acquire) || (isBypassed && tailsOn);

    // Bei hartem Bypass (TAILS = OFF) sofort durchschleifen
    if (isBypassed && ! tailsOn)
        return;

    const float sr    = static_cast<float> (currentSampleRateHz.load());
    const float bpm   = juce::jmax (20.0f, currentBpm.load());
    const bool synced = globalSyncEnabled.load (std::memory_order_acquire);
    const bool freeze = (params.freeze->load() > 0.5f);

    // LFOs & Envelope Follower pro Block updaten
    const float blockSec = static_cast<float> (numSamples) / sr;
    lfo1Phase = std::fmod (lfo1Phase + params.lfo1Rate->load() * blockSec, 1.0f);
    lfo2Phase = std::fmod (lfo2Phase + params.lfo2Rate->load() * blockSec, 1.0f);

    auto evalLfo = [] (float ph, int shape) -> float
    {
        switch (shape)
        {
            case 0: return std::sin (ph * juce::MathConstants<float>::twoPi);
            case 1: return 1.0f - 4.0f * std::abs (std::round (ph - 0.25f) - (ph - 0.25f));
            case 2: return 2.0f * ph - 1.0f;
            case 3: return (ph < 0.5f) ? 1.0f : -1.0f;
            default: return std::sin (ph * juce::MathConstants<float>::twoPi);
        }
    };

    const float lfo1Val = evalLfo (lfo1Phase, static_cast<int> (params.lfo1Shape->load()));
    const float lfo2Val = evalLfo (lfo2Phase, static_cast<int> (params.lfo2Shape->load()));
    liveLfo1Out.store (lfo1Val, std::memory_order_relaxed);
    liveLfo2Out.store (lfo2Val, std::memory_order_relaxed);

    const float envTarget = juce::jlimit (-1.0f, 1.0f, inPeak * params.envSens->load() + params.envBias->load());
    const float envCoeff  = (envTarget > envFollowerState)
                                ? (1.0f - std::exp (-blockSec / juce::jmax (0.001f, params.envAtk->load())))
                                : (1.0f - std::exp (-blockSec / juce::jmax (0.005f, params.envRel->load())));
    envFollowerState += envCoeff * (envTarget - envFollowerState);
    liveEnvFollowerOut.store (envFollowerState, std::memory_order_relaxed);

    // Mod Matrix auswerten
    float modMorph = 0.0f, modWidth = 0.0f;
    std::array<float, 2> modDlyTime { 0.0f, 0.0f };
    std::array<float, 2> modDlyFeed { 0.0f, 0.0f };
    std::array<float, 2> modDecay   { 0.0f, 0.0f };
    std::array<float, 2> modSize    { 0.0f, 0.0f };

    const float baseMorph = params.spaceMorph->load();
    for (size_t m = 0; m < 4; ++m)
    {
        const int src = static_cast<int> (params.modSlots[m].src->load());
        const int dst = static_cast<int> (params.modSlots[m].dst->load());
        const float amt = params.modSlots[m].amount->load();
        if (src == 0 || dst == 0 || std::abs (amt) < 0.001f)
            continue;

        float sVal = 0.0f;
        switch (src)
        {
            case 1: sVal = lfo1Val; break;
            case 2: sVal = lfo2Val; break;
            case 3: sVal = envFollowerState; break;
            case 4: sVal = (baseMorph - 0.5f) * 2.0f; break;
            default: break;
        }

        const float mv = sVal * amt;
        switch (dst)
        {
            case 1:  modMorph      += mv * 0.5f;  break;
            case 2:  modWidth      += mv;         break;
            case 3:  modDlyTime[0] += mv * 80.0f; break;
            case 4:  modDlyFeed[0] += mv * 0.4f;  break;
            case 5:  modDecay[0]   += mv * 4.0f;  break;
            case 6:  modSize[0]    += mv * 0.4f;  break;
            case 7:  modDlyTime[1] += mv * 80.0f; break;
            case 8:  modDlyFeed[1] += mv * 0.4f;  break;
            case 9:  modDecay[1]   += mv * 4.0f;  break;
            case 10: modSize[1]    += mv * 0.4f;  break;
            default: break;
        }
    }

    const float effMorph = juce::jlimit (0.0f, 1.0f, baseMorph + modMorph);
    liveEffectiveMorph.store (effMorph, std::memory_order_relaxed);

    // Space-Parameter vorbereiten
    constexpr float syncBeats[7] = { 0.25f, 0.75f, 0.5f, 1.0f, 1.5f, 2.0f, 4.0f };
    std::array<float, 2> dlySamplesL {}, dlySamplesR {}, dlyFb {}, dlyRevMix {};
    std::array<int, 2>   dlyModes {};
    std::array<bool, 2>  spaceActive {};

    for (size_t s = 0; s < 2; ++s)
    {
        auto& sp  = params.spaces[s];
        auto& dsp = dspSpaces[s];
        spaceActive[s] = (sp.on->load() > 0.5f);

        float timeMs = sp.dlyTime->load();
        if (synced)
        {
            const int syncIdx = juce::jlimit (0, 6, static_cast<int> (sp.dlySync->load()));
            timeMs = (60000.0f / bpm) * syncBeats[syncIdx];
        }
        timeMs = juce::jlimit (1.0f, 2000.0f, timeMs + modDlyTime[s]);

        dlyModes[s]    = static_cast<int> (sp.dlyMode->load());
        dlySamplesL[s] = (timeMs * 0.001f) * sr;
        dlySamplesR[s] = (dlyModes[s] == 3) ? (dlySamplesL[s] * 0.75f) : dlySamplesL[s]; // Dual-Tap Offset

        dlyFb[s]     = freeze ? 0.995f : juce::jlimit (0.0f, 1.05f, sp.dlyFeed->load() + modDlyFeed[s]);
        dlyRevMix[s] = sp.dlyRevMix->load();

        const float toneCut = juce::jmap (sp.dlyTone->load(), 450.0f, 16000.0f);
        dsp.dlyToneFilterL.setCutoffFrequency (toneCut);
        dsp.dlyToneFilterR.setCutoffFrequency (toneCut);

        juce::Reverb::Parameters revP;
        const float decaySec = juce::jlimit (0.1f, 25.0f, sp.revDecay->load() + modDecay[s]);
        revP.roomSize   = freeze ? 1.0f : juce::jlimit (0.05f, 0.99f, 0.25f + (decaySec / 25.0f) * 0.60f + sp.revSize->load() * 0.14f + modSize[s] * 0.1f);
        revP.damping    = sp.revDamp->load();
        revP.wetLevel   = 1.0f;
        revP.dryLevel   = 0.0f;
        revP.width      = 1.0f;
        revP.freezeMode = freeze ? 1.0f : 0.0f;
        dsp.reverb.setParameters (revP);
    }

    // Dry-Kopie für Wet-Processing und Blending anlegen
    juce::AudioBuffer<float> dryBuffer (2, numSamples);
    dryBuffer.copyFrom (0, 0, buffer, 0, 0, numSamples);
    dryBuffer.copyFrom (1, 0, buffer, 1, 0, numSamples);

    // Bei aktivem MUTE IN (oder Bypass mit TAILS = ON) nur den Eingang in die FX-Engines kappen
    if (muteInput)
        buffer.clear();

    juce::AudioBuffer<float> bufA (2, numSamples);
    juce::AudioBuffer<float> bufB (2, numSamples);
    bufA.clear();
    bufB.clear();

    const int sysMode   = static_cast<int> (params.sysMode->load());
    const float drive   = params.sysDrive->load();
    const float crossFb = params.sysCrossFeed->load();

    // Input-Drive anwenden
    if (drive > 0.001f)
    {
        const float g = 1.0f + drive * 6.0f;
        const float normG = 1.0f / std::tanh (g);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* ptr = buffer.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                ptr[i] = std::tanh (ptr[i] * g) * normG;
        }
    }

    // Routing-Verteilung auf Eingang von A und B
    const float splitFreq = params.sysSplitFreq->load();
    crossoverLow.setCutoffFrequency (splitFreq);
    crossoverHigh.setCutoffFrequency (splitFreq);

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = buffer.getSample (0, i);
        const float inR = buffer.getSample (1, i);

        if (sysMode == 3) // Mid / Side
        {
            const float mid  = 0.5f * (inL + inR);
            const float side = 0.5f * (inL - inR);
            bufA.setSample (0, i, mid);  bufA.setSample (1, i, mid);
            bufB.setSample (0, i, side); bufB.setSample (1, i, -side);
        }
        else if (sysMode == 4) // Band Split
        {
            bufA.setSample (0, i, crossoverLow.processSample (0, inL));
            bufA.setSample (1, i, crossoverLow.processSample (1, inR));
            bufB.setSample (0, i, crossoverHigh.processSample (0, inL));
            bufB.setSample (1, i, crossoverHigh.processSample (1, inR));
        }
        else
        {
            bufA.setSample (0, i, inL);
            bufA.setSample (1, i, inR);
            if (sysMode == 0) // Parallel
            {
                bufB.setSample (0, i, inL);
                bufB.setSample (1, i, inR);
            }
        }
    }

    // Hilfsfunktion zur Berechnung einer SpaceEngine (Delay + Reverb)
    auto renderSpaceUnit = [&] (size_t s, juce::AudioBuffer<float>& ioBuf, float crossInL, float crossInR)
    {
        if (! spaceActive[s])
        {
            ioBuf.clear();
            return;
        }

        auto& dsp = dspSpaces[s];
        const float modDepth = params.spaces[s].revMod->load();
        const float preDlySmp = (params.spaces[s].revPreDly->load() * 0.001f) * sr;
        const float mixRatio  = dlyRevMix[s];

        auto* lPtr = ioBuf.getWritePointer (0);
        auto* rPtr = ioBuf.getWritePointer (1);

        // 1. Stereo-Delay + Pre-Delay
        for (int i = 0; i < numSamples; ++i)
        {
            dsp.modPhase = std::fmod (dsp.modPhase + (0.7f / sr), 1.0f);
            const float wobble = std::sin (dsp.modPhase * juce::MathConstants<float>::twoPi) * modDepth * 6.0f;

            const float dL = dsp.dlyLineL.popSample (0, juce::jmax (1.0f, dlySamplesL[s] + wobble));
            const float dR = dsp.dlyLineR.popSample (1, juce::jmax (1.0f, dlySamplesR[s] - wobble));

            float filtL = dsp.dlyToneFilterL.processSample (0, dL);
            float filtR = dsp.dlyToneFilterR.processSample (1, dR);

            if (dlyModes[s] == 2) // Tape Saturation
            {
                filtL = std::tanh (filtL * 1.4f);
                filtR = std::tanh (filtR * 1.4f);
            }

            const float inL = lPtr[i] + crossInL * 0.25f;
            const float inR = rPtr[i] + crossInR * 0.25f;

            if (dlyModes[s] == 1) // Ping-Pong
            {
                dsp.dlyLineL.pushSample (0, inL + filtR * dlyFb[s]);
                dsp.dlyLineR.pushSample (1, inR + filtL * dlyFb[s]);
            }
            else
            {
                dsp.dlyLineL.pushSample (0, inL + filtL * dlyFb[s]);
                dsp.dlyLineR.pushSample (1, inR + filtR * dlyFb[s]);
            }

            // Delay-Anteil + Signal für Reverb-Input
            const float toRevL = inL * (1.0f - mixRatio * 0.35f) + filtL * mixRatio;
            const float toRevR = inR * (1.0f - mixRatio * 0.35f) + filtR * mixRatio;

            dsp.preDlyL.pushSample (0, toRevL);
            dsp.preDlyR.pushSample (1, toRevR);

            lPtr[i] = dsp.preDlyL.popSample (0, juce::jmax (1.0f, preDlySmp));
            rPtr[i] = dsp.preDlyR.popSample (1, juce::jmax (1.0f, preDlySmp));

            dsp.fbStateL = filtL;
            dsp.fbStateR = filtR;
        }

        // 2. Reverb Tank auf das Signal anwenden
        juce::dsp::AudioBlock<float> spaceBlock (ioBuf);
        juce::dsp::ProcessContextReplacing<float> spaceCtx (spaceBlock);
        dsp.reverb.process (spaceCtx);
    };

    // Routing-Reihenfolge ausführen
    if (sysMode == 1) // Serial A -> B
    {
        renderSpaceUnit (0, bufA, crossFbBtoA * crossFb, crossFbBtoA * crossFb);
        bufB.copyFrom (0, 0, bufA, 0, 0, numSamples);
        bufB.copyFrom (1, 0, bufA, 1, 0, numSamples);
        renderSpaceUnit (1, bufB, 0.0f, 0.0f);
    }
    else if (sysMode == 2) // Serial B -> A
    {
        bufB.copyFrom (0, 0, buffer, 0, 0, numSamples);
        bufB.copyFrom (1, 0, buffer, 1, 0, numSamples);
        renderSpaceUnit (1, bufB, crossFbAtoB * crossFb, crossFbAtoB * crossFb);
        bufA.copyFrom (0, 0, bufB, 0, 0, numSamples);
        bufA.copyFrom (1, 0, bufB, 1, 0, numSamples);
        renderSpaceUnit (0, bufA, 0.0f, 0.0f);
    }
    else
    {
        renderSpaceUnit (0, bufA, crossFbBtoA * crossFb, crossFbBtoA * crossFb);
        renderSpaceUnit (1, bufB, crossFbAtoB * crossFb, crossFbAtoB * crossFb);
    }

    crossFbAtoB = dspSpaces[0].fbStateL;
    crossFbBtoA = dspSpaces[1].fbStateR;

    // Scene Morphing & Zusammenmischen in Wet-Buffer
    const float lvlA    = params.spaceALevel->load();
    const float lvlB    = params.spaceBLevel->load();
    const float weightA = lvlA * (1.0f - effMorph);
    const float weightB = lvlB * effMorph;

    for (int ch = 0; ch < 2; ++ch)
    {
        auto* wetPtr = buffer.getWritePointer (ch);
        const auto* aPtr = bufA.getReadPointer (ch);
        const auto* bPtr = bufB.getReadPointer (ch);

        for (int i = 0; i < numSamples; ++i)
            wetPtr[i] = aPtr[i] * weightA + bPtr[i] * weightB;
    }

    // 4-Band Wet-EQ
    updateWetEqFilters();
    juce::dsp::AudioBlock<float> wetBlock (buffer);
    juce::dsp::ProcessContextReplacing<float> wetCtx (wetBlock);
    wetEqLow.process (wetCtx);
    wetEqMid1.process (wetCtx);
    wetEqMid2.process (wetCtx);
    wetEqHigh.process (wetCtx);

    // Stereo-Width, Ducking, Gate & Dry/Wet Blend
    const float width     = params.outWidth->load();
    const float dryGain   = params.mixDry->load();
    const float wetGain   = params.mixWet->load();
    const float outGain   = juce::Decibels::decibelsToGain (params.outGainDb->load());
    const float duckAmt   = params.duckAmt->load();
    const float gateThrLin = juce::Decibels::decibelsToGain (params.gateThr->load());

    const float targetDuck = 1.0f - juce::jlimit (0.0f, 0.95f, inPeak * 2.5f * duckAmt);
    duckGainState += (1.0f - std::exp (-blockSec / juce::jmax (0.01f, params.duckRel->load()))) * (targetDuck - duckGainState);

    const float targetGate = (params.gateThr->load() <= -59.5f || inPeak >= gateThrLin) ? 1.0f : 0.0f;
    gateGainState += (1.0f - std::exp (-blockSec / juce::jmax (0.005f, params.gateRel->load()))) * (targetGate - gateGainState);

    const float dynamicWetScalar = wetGain * duckGainState * gateGainState;

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getWritePointer (1);
    const auto* dL = dryBuffer.getReadPointer (0);
    const auto* dR = dryBuffer.getReadPointer (1);

    const float effDryGain = isBypassed ? 1.0f : dryGain;
    const float effOutGain = isBypassed ? 1.0f : outGain;

    for (int i = 0; i < numSamples; ++i)
    {
        const float mid  = 0.5f * (outL[i] + outR[i]);
        const float side = 0.5f * (outL[i] - outR[i]) * width;

        const float wL = (mid + side) * dynamicWetScalar;
        const float wR = (mid - side) * dynamicWetScalar;

        outL[i] = (dL[i] * effDryGain + wL) * effOutGain;
        outR[i] = (dR[i] * effDryGain + wR) * effOutGain;
    }

    // Output-Peaks & Scope-Ringbuffer
    const float peakL = buffer.getMagnitude (0, 0, numSamples);
    const float peakR = buffer.getMagnitude (1, 0, numSamples);

    float prevL = outputPeakL.load (std::memory_order_relaxed);
    while (peakL > prevL && ! outputPeakL.compare_exchange_weak (prevL, peakL, std::memory_order_relaxed)) {}

    float prevR = outputPeakR.load (std::memory_order_relaxed);
    while (peakR > prevR && ! outputPeakR.compare_exchange_weak (prevR, peakR, std::memory_order_relaxed)) {}
}

// ==============================================================================
// Space Copy / Swap / Reset / Randomize
// ==============================================================================
void CyberverbX1AudioProcessor::copySpaceParameters (int sourceSpace, int targetSpace)
{
    if (sourceSpace == targetSpace) return;
    const auto srcPrefix = (sourceSpace == 1) ? "a_" : "b_";
    const auto dstPrefix = (targetSpace == 1) ? "a_" : "b_";

    for (auto* param : getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = rp->getParameterID();
            if (id.startsWith (srcPrefix))
                if (auto* tp = apvts.getParameter (dstPrefix + id.substring (2)))
                    tp->setValueNotifyingHost (rp->getValue());
        }
    }
}

void CyberverbX1AudioProcessor::swapSpaceParameters()
{
    for (auto* param : getParameters())
    {
        if (auto* p1 = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = p1->getParameterID();
            if (id.startsWith ("a_"))
            {
                if (auto* p2 = apvts.getParameter ("b_" + id.substring (2)))
                {
                    const float v1 = p1->getValue();
                    const float v2 = p2->getValue();
                    p1->setValueNotifyingHost (v2);
                    p2->setValueNotifyingHost (v1);
                }
            }
        }
    }
}

void CyberverbX1AudioProcessor::resetSpaceParameters (int space)
{
    const auto prefix = (space == 1) ? "a_" : "b_";
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            if (rp->getParameterID().startsWith (prefix))
                rp->setValueNotifyingHost (rp->getDefaultValue());
}

void CyberverbX1AudioProcessor::randomizeSpaceParameters (int space)
{
    const auto prefix = (space == 1) ? "a_" : "b_";
    auto& rng = juce::Random::getSystemRandom();

    for (auto* param : getParameters())
    {
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
        {
            const auto id = rp->getParameterID();
            if (id.startsWith (prefix) && ! id.endsWith ("_on"))
            {
                if (id.endsWith ("_feed"))
                    rp->setValueNotifyingHost (rng.nextFloat() * 0.65f);
                else
                    rp->setValueNotifyingHost (rng.nextFloat());
            }
        }
    }
}

// ==============================================================================
// Preset & State Management
// ==============================================================================
juce::File CyberverbX1AudioProcessor::getPresetDirectory() const
{
    auto baseDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    auto dir = baseDir.getChildFile ("Application Support").getChildFile ("CyberverbX1").getChildFile ("Presets");
   #else
    auto dir = baseDir.getChildFile ("CyberverbX1").getChildFile ("Presets");
   #endif
    if (! dir.exists()) dir.createDirectory();
    return dir;
}

void CyberverbX1AudioProcessor::ensureDefaultPresetExists()
{
    getFactoryPresetDirectory().createDirectory();
    getUserPresetDirectory().createDirectory();

    const auto initFile = getPresetDirectory().getChildFile ("Init.xml");
    if (! initFile.existsAsFile())
    {
        auto state = apvts.copyState();
        if (std::unique_ptr<juce::XmlElement> xml { state.createXml() })
        {
            xml->setAttribute ("presetName", "Init");
            xml->writeTo (initFile);
        }
    }
}

void CyberverbX1AudioProcessor::installFactoryPresets (bool overwriteExisting)
{
    const auto factoryRoot = getFactoryPresetDirectory();
    factoryRoot.createDirectory();

    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String origName (BinaryData::originalFilenames[i]);
        if (! origName.endsWithIgnoreCase (".xml")) continue;

        int dataSize = 0;
        const char* rawData = BinaryData::getNamedResource (BinaryData::namedResourceList[i], dataSize);
        if (rawData == nullptr || dataSize <= 0) continue;

        if (auto xml = juce::XmlDocument::parse (juce::String::fromUTF8 (rawData, dataSize)))
        {
            if (! xml->hasTagName (apvts.state.getType()) && ! xml->hasTagName ("PARAMETERS"))
                continue;

            xml->setTagName (apvts.state.getType());

            const auto presetName = xml->getStringAttribute ("presetName",
                juce::File::createFileWithoutCheckingPath (origName).getFileNameWithoutExtension());
            const auto targetFile = factoryRoot.getChildFile (juce::File::createLegalFileName (presetName) + ".xml");
            if (overwriteExisting || ! targetFile.existsAsFile())
                xml->writeTo (targetFile);
        }
    }
}

juce::Array<CyberverbX1AudioProcessor::PresetEntry> CyberverbX1AudioProcessor::getAllPresetEntries() const
{
    juce::Array<PresetEntry> entries;
    const auto rootDir = getPresetDirectory();

    PresetEntry initEntry;
    initEntry.name = "Init"; initEntry.author = "Factory";
    initEntry.file = rootDir.getChildFile ("Init.xml");
    initEntry.isFactory = true; initEntry.isInit = true;
    entries.add (initEntry);

    auto scan = [&] (const juce::File& dir, bool isFac)
    {
        if (! dir.isDirectory()) return;
        auto files = dir.findChildFiles (juce::File::findFiles, true, "*.xml");
        files.sort();
        for (const auto& f : files)
        {
            PresetEntry e;
            e.file = f; e.isFactory = isFac;
            const auto rel = f.getParentDirectory().getRelativePathFrom (rootDir).replaceCharacter ('\\', '/');
            e.category = (rel == ".") ? "" : rel;
            e.name     = f.getFileNameWithoutExtension();
            if (auto xml = juce::XmlDocument::parse (f))
            {
                e.name   = xml->getStringAttribute ("presetName", e.name);
                e.author = xml->getStringAttribute ("presetAuthor", isFac ? "NetAndif" : "User");
                e.notes  = xml->getStringAttribute ("presetNotes", "");
            }
            entries.add (e);
        }
    };

    scan (getFactoryPresetDirectory(), true);
    scan (getUserPresetDirectory(), false);
    return entries;
}

juce::StringArray CyberverbX1AudioProcessor::getAvailableCategories() const
{
    juce::StringArray cats { "Factory", "User" };
    const auto rootDir = getPresetDirectory();
    for (const auto& d : getUserPresetDirectory().findChildFiles (juce::File::findDirectories, true))
    {
        const auto rel = d.getRelativePathFrom (rootDir).replaceCharacter ('\\', '/');
        if (rel.isNotEmpty()) cats.addIfNotAlreadyThere (rel);
    }
    return cats;
}

bool CyberverbX1AudioProcessor::savePreset (const juce::String& presetName, const juce::String& category,
                                            const juce::String& author, const juce::String& notes)
{
    const auto cleanName = juce::File::createLegalFileName (presetName.trim());
    if (cleanName.isEmpty() || cleanName.equalsIgnoreCase ("Init")) return false;

    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr) return false;

    auto cleanCat = category.trim().replaceCharacter ('\\', '/');
    if (cleanCat.isEmpty() || cleanCat.startsWithIgnoreCase ("Factory")) cleanCat = "User";
    else if (! cleanCat.startsWithIgnoreCase ("User")) cleanCat = "User/" + cleanCat;

    auto targetDir = getPresetDirectory();
    for (const auto& part : juce::StringArray::fromTokens (cleanCat, "/", ""))
        if (part.isNotEmpty()) targetDir = targetDir.getChildFile (juce::File::createLegalFileName (part));

    targetDir.createDirectory();
    xml->setAttribute ("presetName", cleanName);
    xml->setAttribute ("presetCategory", cleanCat);
    xml->setAttribute ("presetAuthor", author);
    xml->setAttribute ("presetNotes", notes);

    const auto targetFile = targetDir.getChildFile (cleanName + ".xml");
    if (xml->writeTo (targetFile))
    {
        currentPresetName = cleanName; currentPresetCategory = cleanCat;
        currentPresetAuthor = author; currentPresetNotes = notes;
        currentPresetFile = targetFile;
        currentPresetIsSaved = true; currentPresetIsFactory = false;
        return true;
    }
    return false;
}

bool CyberverbX1AudioProcessor::loadPresetByName (const juce::String& presetName)
{
    if (presetName.equalsIgnoreCase ("Init")) { loadInitPatch(); return true; }
    for (const auto& e : getAllPresetEntries())
        if (e.name.equalsIgnoreCase (presetName) && e.file.existsAsFile())
            return loadPresetFromFile (e.file, false);
    return false;
}

bool CyberverbX1AudioProcessor::loadPresetEntry (const PresetEntry& entry)
{
    if (entry.isInit) { loadInitPatch(); return true; }
    return loadPresetFromFile (entry.file, false, entry.category);
}

bool CyberverbX1AudioProcessor::loadPresetFromFile (const juce::File& file, bool importToLibrary, const juce::String& targetCategory)
{
    if (! file.existsAsFile()) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;

    // Bei aktivem MIX LOCK den aktuellen Dry/Wet-Wert merken und nach dem Preset-Laden wiederherstellen
    const float lockedDry = params.mixDry->load();
    const float lockedWet = params.mixWet->load();

    apvts.replaceState (juce::ValueTree::fromXml (*xml));

    if (mixLockEnabled.load (std::memory_order_acquire))
    {
        if (auto* pDry = apvts.getParameter ("mix_dry")) pDry->setValueNotifyingHost (pDry->convertTo0to1 (lockedDry));
        if (auto* pWet = apvts.getParameter ("mix_wet")) pWet->setValueNotifyingHost (pWet->convertTo0to1 (lockedWet));
    }

    currentPresetName      = xml->getStringAttribute ("presetName", file.getFileNameWithoutExtension());
    currentPresetCategory  = targetCategory;
    currentPresetFile      = file;
    currentPresetIsFactory = file.isAChildOf (getFactoryPresetDirectory());
    currentPresetIsSaved   = ! currentPresetIsFactory && file.isAChildOf (getPresetDirectory());
    currentPresetAuthor    = xml->getStringAttribute ("presetAuthor", "");
    currentPresetNotes     = xml->getStringAttribute ("presetNotes", "");

    if (importToLibrary && ! currentPresetName.equalsIgnoreCase ("Init"))
        savePreset (currentPresetName, "User", currentPresetAuthor, currentPresetNotes);

    resetAllPickupLocks();
    flushAudioTails();
    return true;
}

bool CyberverbX1AudioProcessor::renameOrMoveCurrentUserPreset (const juce::String& newName, const juce::String& newCategory)
{
    if (! currentPresetIsSaved || currentPresetIsFactory || ! currentPresetFile.existsAsFile()) return false;
    const auto oldFile = currentPresetFile;
    if (savePreset (newName, newCategory, currentPresetAuthor, currentPresetNotes))
    {
        if (oldFile != currentPresetFile) oldFile.deleteFile();
        return true;
    }
    return false;
}

bool CyberverbX1AudioProcessor::deleteCurrentUserPreset()
{
    if (! currentPresetIsSaved || currentPresetIsFactory || ! currentPresetFile.existsAsFile()) return false;
    if (currentPresetFile.moveToTrash())
    {
        loadInitPatch();
        return true;
    }
    return false;
}

void CyberverbX1AudioProcessor::restoreFactoryPresets() { installFactoryPresets (true); }

void CyberverbX1AudioProcessor::loadInitPatch()
{
    const float lockedDry = params.mixDry->load();
    const float lockedWet = params.mixWet->load();

    for (auto* param : getParameters())
        param->setValueNotifyingHost (param->getDefaultValue());

    if (mixLockEnabled.load (std::memory_order_acquire))
    {
        if (auto* pDry = apvts.getParameter ("mix_dry")) pDry->setValueNotifyingHost (pDry->convertTo0to1 (lockedDry));
        if (auto* pWet = apvts.getParameter ("mix_wet")) pWet->setValueNotifyingHost (pWet->convertTo0to1 (lockedWet));
    }

    currentPresetName = "Init"; currentPresetCategory = "";
    currentPresetIsSaved = false; currentPresetIsFactory = true;
    resetAllPickupLocks();
    flushAudioTails();
}

void CyberverbX1AudioProcessor::flushAudioTails() { flushRequested.store (true); }

const juce::String CyberverbX1AudioProcessor::getName() const { return JucePlugin_Name; }
bool CyberverbX1AudioProcessor::acceptsMidi() const { return true; }
bool CyberverbX1AudioProcessor::producesMidi() const { return false; }
bool CyberverbX1AudioProcessor::isMidiEffect() const { return false; }
double CyberverbX1AudioProcessor::getTailLengthSeconds() const { return 10.0; }
int CyberverbX1AudioProcessor::getNumPrograms() { return 1; }
int CyberverbX1AudioProcessor::getCurrentProgram() { return 0; }
void CyberverbX1AudioProcessor::setCurrentProgram (int) {}
const juce::String CyberverbX1AudioProcessor::getProgramName (int) { return {}; }
void CyberverbX1AudioProcessor::changeProgramName (int, const juce::String&) {}
bool CyberverbX1AudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* CyberverbX1AudioProcessor::createEditor()
{
    return new CyberverbX1AudioProcessorEditor (*this);
}

void CyberverbX1AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (std::unique_ptr<juce::XmlElement> xml { state.createXml() })
    {
        xml->setAttribute ("presetName", currentPresetName);
        xml->setAttribute ("presetCategory", currentPresetCategory);
        xml->setAttribute ("mixLockEnabled", isMixLockEnabled());
        xml->setAttribute ("globalSyncEnabled", isGlobalSyncEnabled());
        xml->setAttribute ("midiPickupEnabled", isMidiPickupEnabled());
        xml->setAttribute ("tailsEnabled", isTailsEnabled());
        xml->setAttribute ("linkABEnabled", isLinkABEnabled());

        for (int k = 0; k < numAssignKnobs; ++k)
        {
            xml->setAttribute ("assignMidiCC_" + juce::String (k + 1), assignKnobMidiCC[static_cast<size_t> (k)].load());
            xml->setAttribute ("assignMidiMode_" + juce::String (k + 1), assignKnobCCMode[static_cast<size_t> (k)].load());
        }
        copyXmlToBinary (*xml, destData);
    }
}

void CyberverbX1AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
    {
        currentPresetName     = xmlState->getStringAttribute ("presetName", "Init");
        currentPresetCategory = xmlState->getStringAttribute ("presetCategory", "");
        setMixLockEnabled (xmlState->getBoolAttribute ("mixLockEnabled", false));
        setGlobalSyncEnabled (xmlState->getBoolAttribute ("globalSyncEnabled", false));
        setMidiPickupEnabled (xmlState->getBoolAttribute ("midiPickupEnabled", true));
        setTailsEnabled (xmlState->getBoolAttribute ("tailsEnabled", true));
        setLinkABEnabled (xmlState->getBoolAttribute ("linkABEnabled", false));

        for (int k = 0; k < numAssignKnobs; ++k)
        {
            assignKnobMidiCC[static_cast<size_t> (k)].store (xmlState->getIntAttribute ("assignMidiCC_" + juce::String (k + 1), -1));
            assignKnobCCMode[static_cast<size_t> (k)].store (xmlState->getIntAttribute ("assignMidiMode_" + juce::String (k + 1), 0));
        }
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
    }
}

juce::File CyberverbX1AudioProcessor::getMidiMapDirectory() const
{
    auto baseDir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);
   #if JUCE_MAC
    auto dir = baseDir.getChildFile ("Application Support").getChildFile ("CyberverbX1").getChildFile ("MidiMaps");
   #else
    auto dir = baseDir.getChildFile ("CyberverbX1").getChildFile ("MidiMaps");
   #endif
    if (! dir.exists()) dir.createDirectory();
    return dir;
}

bool CyberverbX1AudioProcessor::saveMidiMappingToFile (const juce::File& file) const
{
    auto xml = std::make_unique<juce::XmlElement> ("CYBERVERB_MIDI_MAP");
    xml->setAttribute ("pickupEnabled", midiPickupEnabled.load());
    for (int k = 0; k < numAssignKnobs; ++k)
    {
        auto* slot = xml->createNewChildElement ("AssignSlot");
        slot->setAttribute ("index", k + 1);
        slot->setAttribute ("cc", assignKnobMidiCC[static_cast<size_t> (k)].load());
        slot->setAttribute ("mode", assignKnobCCMode[static_cast<size_t> (k)].load());
    }
    return xml->writeTo (file);
}

bool CyberverbX1AudioProcessor::loadMidiMappingFromFile (const juce::File& file)
{
    if (! file.existsAsFile()) return false;
    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse (file));
    if (xml == nullptr || ! xml->hasTagName ("CYBERVERB_MIDI_MAP")) return false;

    clearAllMidiMappings();
    midiPickupEnabled.store (xml->getBoolAttribute ("pickupEnabled", true));
    for (auto* child : xml->getChildWithTagNameIterator ("AssignSlot"))
    {
        const int idx = child->getIntAttribute ("index", 0) - 1;
        const int cc  = child->getIntAttribute ("cc", -1);
        if (idx >= 0 && idx < numAssignKnobs && cc >= 0 && cc <= 127)
        {
            assignKnobMidiCC[static_cast<size_t> (idx)].store (cc);
            assignKnobCCMode[static_cast<size_t> (idx)].store (juce::jlimit (0, 2, child->getIntAttribute ("mode", 0)));
        }
    }
    return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CyberverbX1AudioProcessor();
}