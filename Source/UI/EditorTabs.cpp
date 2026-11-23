#include "../PluginEditor.h"
#include "../PluginProcessor.h"

void CyberverbX1AudioProcessorEditor::initTabControls()
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    auto setupKnob = [this] (juce::Slider& slider, const juce::String& paramID, const juce::String& title,
                             juce::Colour accent, bool bipolar, std::function<juce::String (double)> fmt)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.getProperties().set ("isBipolar", bipolar);

        if (auto* p = audioProcessor.apvts.getParameter (paramID))
            slider.setDoubleClickReturnValue (true, p->getDefaultValue());

        slider.onValueChange = [this, &slider, paramID, title, fmt]
        {
            setKeypadTarget (paramID, title);
            triggerLcdNotification (title, fmt (slider.getValue()));
        };

        addChildComponent (slider);
    };

    // --- Tab 1: ROUTING ---
    routingControls.sysModeBox.addItemList ({ "Parallel", "Serial A > B", "Serial B > A", "Mid / Side", "Band Split" }, 1);
    routingControls.sysModeBox.onChange = [this] { triggerLcdNotification ("ROUTING MODE", routingControls.sysModeBox.getText()); };
    addChildComponent (routingControls.sysModeBox);
    routingControls.sysModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "sys_mode", routingControls.sysModeBox);

    setupKnob (routingControls.splitFreqKnob, "sys_split_freq", "SPLIT FREQ", colA, false, [] (double v) { return juce::String::formatted ("%d HZ", juce::roundToInt (v)); });
    setupKnob (routingControls.driveKnob,     "sys_drive",      "INPUT DRIVE", colA, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
    setupKnob (routingControls.crossFeedKnob, "sys_cross_feed", "CROSS FEED", colA, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
    setupKnob (routingControls.duckAmtKnob,   "duck_amt",       "DUCKING AMT", colB, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
    setupKnob (routingControls.duckRelKnob,   "duck_rel",       "DUCK RELEASE", colB, false, [] (double v) { return juce::String::formatted ("%d MS", juce::roundToInt (v * 1000.0)); });
    setupKnob (routingControls.gateThrKnob,   "gate_thr",       "GATE THRESH", colB, false, [] (double v) { return juce::String::formatted ("%.1f DB", v); });
    setupKnob (routingControls.gateRelKnob,   "gate_rel",       "GATE RELEASE", colB, false, [] (double v) { return juce::String::formatted ("%d MS", juce::roundToInt (v * 1000.0)); });

    routingControls.splitFreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "sys_split_freq", routingControls.splitFreqKnob);
    routingControls.driveAtt     = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "sys_drive",      routingControls.driveKnob);
    routingControls.crossFeedAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "sys_cross_feed", routingControls.crossFeedKnob);
    routingControls.duckAmtAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "duck_amt",       routingControls.duckAmtKnob);
    routingControls.duckRelAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "duck_rel",       routingControls.duckRelKnob);
    routingControls.gateThrAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "gate_thr",       routingControls.gateThrKnob);
    routingControls.gateRelAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "gate_rel",       routingControls.gateRelKnob);

    // --- Tab 2 & 3: SPACE A & SPACE B ---
    for (size_t s = 0; s < 2; ++s)
    {
        auto& sc = spaceTabControls[s];
        const juce::String pre   = (s == 0) ? "a_" : "b_";
        const juce::String lbl   = (s == 0) ? "SPACE A // " : "SPACE B // ";
        const auto accent        = (s == 0) ? colA : colB;

        sc.dlyModeBox.addItemList ({ "Stereo", "Ping-Pong", "Tape Echo", "Dual Tap" }, 1);
        sc.dlyModeBox.onChange = [this, &sc, lbl] { triggerLcdNotification (lbl + "DLY MODE", sc.dlyModeBox.getText()); };
        addChildComponent (sc.dlyModeBox);

        sc.dlySyncBox.addItemList ({ "1/16", "1/8 Dot", "1/8", "1/4", "1/4 Dot", "1/2", "1/1" }, 1);
        sc.dlySyncBox.onChange = [this, &sc, lbl] { triggerLcdNotification (lbl + "DLY SYNC", sc.dlySyncBox.getText()); };
        addChildComponent (sc.dlySyncBox);

        if (s == 0)
            sc.revAlgoBox.addItemList ({ "Plate", "Room", "Hall", "Chamber", "Vintage Spring" }, 1);
        else
            sc.revAlgoBox.addItemList ({ "Shimmer Cloud", "Cyber Hall", "Granular Mist", "Dark Void", "Reverse Bloom" }, 1);
        sc.revAlgoBox.onChange = [this, &sc, lbl] { triggerLcdNotification (lbl + "REV ALGO", sc.revAlgoBox.getText()); };
        addChildComponent (sc.revAlgoBox);

        sc.dlyModeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, pre + "dly_mode", sc.dlyModeBox);
        sc.dlySyncAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, pre + "dly_sync", sc.dlySyncBox);
        sc.revAlgoAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, pre + "rev_algo", sc.revAlgoBox);

        setupKnob (sc.dlyTimeKnob,   pre + "dly_time",    lbl + "DLY TIME",  colA, false, [] (double v) { return juce::String::formatted ("%.1f MS", v); });
        setupKnob (sc.dlyToneKnob,   pre + "dly_tone",    lbl + "DLY TONE",  colA, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
        setupKnob (sc.dlyRevMixKnob, pre + "dly_rev_mix", lbl + "DLY > REV", colA, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });

        setupKnob (sc.preDlyKnob,    pre + "rev_predly",  lbl + "PRE-DELAY", accent, false, [] (double v) { return juce::String::formatted ("%.1f MS", v); });
        setupKnob (sc.diffuseKnob,   pre + "rev_diffuse", lbl + (s == 0 ? "DIFFUSION" : "SHIMMER"), accent, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
        setupKnob (sc.modDepthKnob,  pre + "rev_mod",     lbl + "MOD DEPTH", accent, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });

        sc.dlyTimeAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "dly_time",    sc.dlyTimeKnob);
        sc.dlyToneAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "dly_tone",    sc.dlyToneKnob);
        sc.dlyRevMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "dly_rev_mix", sc.dlyRevMixKnob);
        sc.preDlyAtt    = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "rev_predly",  sc.preDlyKnob);
        sc.diffuseAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "rev_diffuse", sc.diffuseKnob);
        sc.modDepthAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, pre + "rev_mod",     sc.modDepthKnob);
    }

    // --- Tab 4: MODULATION ---
    const juce::StringArray shapes = { "Sine", "Triangle", "Saw Up", "Square", "Sample & Hold" };
    modDynControls.lfo1ShapeBox.addItemList (shapes, 1);
    modDynControls.lfo2ShapeBox.addItemList (shapes, 1);
    addChildComponent (modDynControls.lfo1ShapeBox);
    addChildComponent (modDynControls.lfo2ShapeBox);
    modDynControls.lfo1ShapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "lfo1_shape", modDynControls.lfo1ShapeBox);
    modDynControls.lfo2ShapeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "lfo2_shape", modDynControls.lfo2ShapeBox);

    setupKnob (modDynControls.lfo1RateKnob, "lfo1_rate", "LFO 1 RATE", colA, false, [] (double v) { return juce::String::formatted ("%.2f HZ", v); });
    setupKnob (modDynControls.lfo2RateKnob, "lfo2_rate", "LFO 2 RATE", colA, false, [] (double v) { return juce::String::formatted ("%.2f HZ", v); });
    setupKnob (modDynControls.envSensKnob,  "env_sens",  "ENV SENS",   colB, false, [] (double v) { return juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)); });
    setupKnob (modDynControls.envAtkKnob,   "env_atk",   "ENV ATTACK", colB, false, [] (double v) { return juce::String::formatted ("%d MS", juce::roundToInt (v * 1000.0)); });
    setupKnob (modDynControls.envRelKnob,   "env_rel",   "ENV RELEASE",colB, false, [] (double v) { return juce::String::formatted ("%d MS", juce::roundToInt (v * 1000.0)); });
    setupKnob (modDynControls.envBiasKnob,  "env_bias",  "ENV BIAS",   colB, true,  [] (double v) { return juce::String::formatted ("%+d%%", juce::roundToInt (v * 100.0)); });

    modDynControls.lfo1RateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lfo1_rate", modDynControls.lfo1RateKnob);
    modDynControls.lfo2RateAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "lfo2_rate", modDynControls.lfo2RateKnob);
    modDynControls.envSensAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "env_sens",  modDynControls.envSensKnob);
    modDynControls.envAtkAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "env_atk",   modDynControls.envAtkKnob);
    modDynControls.envRelAtt   = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "env_rel",   modDynControls.envRelKnob);
    modDynControls.envBiasAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "env_bias",  modDynControls.envBiasKnob);

    // --- Tab 5: EQ ---
    auto fmtHz = [] (double v) { return v < 1000.0 ? juce::String::formatted ("%d HZ", juce::roundToInt (v)) : juce::String::formatted ("%.2f KHZ", v / 1000.0); };
    auto fmtDb = [] (double v) { return juce::String::formatted ("%+.1f DB", v); };

    setupKnob (eqControls.lowFreqKnob,  "eq_low_freq",  "EQ LOW FREQ",   colA, false, fmtHz);
    setupKnob (eqControls.lowGainKnob,  "eq_low_gain",  "EQ LOW GAIN",   colA, true,  fmtDb);
    setupKnob (eqControls.mid1FreqKnob, "eq_mid1_freq", "EQ MID 1 FREQ", colA, false, fmtHz);
    setupKnob (eqControls.mid1GainKnob, "eq_mid1_gain", "EQ MID 1 GAIN", colA, true,  fmtDb);
    setupKnob (eqControls.mid2FreqKnob, "eq_mid2_freq", "EQ MID 2 FREQ", colB, false, fmtHz);
    setupKnob (eqControls.mid2GainKnob, "eq_mid2_gain", "EQ MID 2 GAIN", colB, true,  fmtDb);
    setupKnob (eqControls.highFreqKnob, "eq_high_freq", "EQ HIGH FREQ",  colB, false, fmtHz);
    setupKnob (eqControls.highGainKnob, "eq_high_gain", "EQ HIGH GAIN",  colB, true,  fmtDb);

    eqControls.lowFreqAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_low_freq",  eqControls.lowFreqKnob);
    eqControls.lowGainAtt  = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_low_gain",  eqControls.lowGainKnob);
    eqControls.mid1FreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_mid1_freq", eqControls.mid1FreqKnob);
    eqControls.mid1GainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_mid1_gain", eqControls.mid1GainKnob);
    eqControls.mid2FreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_mid2_freq", eqControls.mid2FreqKnob);
    eqControls.mid2GainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_mid2_gain", eqControls.mid2GainKnob);
    eqControls.highFreqAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_high_freq", eqControls.highFreqKnob);
    eqControls.highGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "eq_high_gain", eqControls.highGainKnob);

    // --- Tab 6: MATRIX ---
    const juce::StringArray modSrc = { "Off", "LFO 1", "LFO 2", "Env Follower", "Space Morph" };
    const juce::StringArray modDst = { "Off", "Space Morph", "Stereo Width", "A: Delay Time", "A: Delay Feed", "A: Decay", "A: Size", "B: Delay Time", "B: Delay Feed", "B: Decay", "B: Size" };

    for (size_t m = 0; m < 4; ++m)
    {
        auto& slot = modMatrixSlots[m];
        const auto mp = "mod" + juce::String (static_cast<int> (m + 1)) + "_";

        slot.srcBox.addItemList (modSrc, 1);
        slot.dstBox.addItemList (modDst, 1);
        addChildComponent (slot.srcBox);
        addChildComponent (slot.dstBox);

        setupKnob (slot.amtKnob, mp + "amt", "MOD " + juce::String (static_cast<int> (m + 1)) + " AMT", colA, true,
                   [] (double v) { return juce::String::formatted ("%+d%%", juce::roundToInt (v * 100.0)); });

        slot.srcAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, mp + "src", slot.srcBox);
        slot.dstAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, mp + "dst", slot.dstBox);
        slot.amtAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>   (audioProcessor.apvts, mp + "amt", slot.amtKnob);
    }

    updateTabVisibility();
}

void CyberverbX1AudioProcessorEditor::updateTabVisibility()
{
    const bool r = (currentTab == ActiveTab::Routing);
    routingControls.sysModeBox.setVisible (r);
    routingControls.splitFreqKnob.setVisible (r);
    routingControls.driveKnob.setVisible (r);
    routingControls.crossFeedKnob.setVisible (r);
    routingControls.duckAmtKnob.setVisible (r);
    routingControls.duckRelKnob.setVisible (r);
    routingControls.gateThrKnob.setVisible (r);
    routingControls.gateRelKnob.setVisible (r);

    for (size_t s = 0; s < 2; ++s)
    {
        const bool vis = (s == 0 && currentTab == ActiveTab::SpaceA) || (s == 1 && currentTab == ActiveTab::SpaceB);
        auto& sc = spaceTabControls[s];
        sc.dlyModeBox.setVisible (vis);
        sc.dlySyncBox.setVisible (vis);
        sc.revAlgoBox.setVisible (vis);
        sc.dlyTimeKnob.setVisible (vis);
        sc.dlyToneKnob.setVisible (vis);
        sc.dlyRevMixKnob.setVisible (vis);
        sc.preDlyKnob.setVisible (vis);
        sc.diffuseKnob.setVisible (vis);
        sc.modDepthKnob.setVisible (vis);
    }

    const bool m = (currentTab == ActiveTab::Modulation);
    modDynControls.lfo1ShapeBox.setVisible (m);
    modDynControls.lfo2ShapeBox.setVisible (m);
    modDynControls.lfo1RateKnob.setVisible (m);
    modDynControls.lfo2RateKnob.setVisible (m);
    modDynControls.envSensKnob.setVisible (m);
    modDynControls.envAtkKnob.setVisible (m);
    modDynControls.envRelKnob.setVisible (m);
    modDynControls.envBiasKnob.setVisible (m);

    const bool e = (currentTab == ActiveTab::EQ);
    eqControls.lowFreqKnob.setVisible (e);
    eqControls.lowGainKnob.setVisible (e);
    eqControls.mid1FreqKnob.setVisible (e);
    eqControls.mid1GainKnob.setVisible (e);
    eqControls.mid2FreqKnob.setVisible (e);
    eqControls.mid2GainKnob.setVisible (e);
    eqControls.highFreqKnob.setVisible (e);
    eqControls.highGainKnob.setVisible (e);

    const bool mx = (currentTab == ActiveTab::Matrix);
    for (auto& slot : modMatrixSlots)
    {
        slot.srcBox.setVisible (mx);
        slot.dstBox.setVisible (mx);
        slot.amtKnob.setVisible (mx);
    }
}