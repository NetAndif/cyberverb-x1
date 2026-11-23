#include "../PluginEditor.h"
#include "../PluginProcessor.h"

namespace
{
    void drawKnobLabel (juce::Graphics& g, const juce::Slider& knob, const juce::String& text)
    {
        const auto kb = knob.getBounds().toFloat();
        auto labelRect = juce::Rectangle<float> (kb.getX() - 10.0f, kb.getBottom() - 2.0f, kb.getWidth() + 20.0f, 15.0f);
        g.setColour (CyberverbColours::textPrimary.withAlpha (0.85f));
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (text, labelRect, juce::Justification::centred);
    }
}

void CyberverbX1AudioProcessorEditor::drawLcdScreen (juce::Graphics& g, juce::Rectangle<float> lcdBounds)
{
    auto screen = lcdBounds.reduced (7.0f, 6.0f);
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    g.setColour (colA.withAlpha (0.045f));
    for (float y = screen.getY(); y < screen.getBottom(); y += 3.0f)
        g.drawHorizontalLine (juce::roundToInt (y), screen.getX(), screen.getRight());

    auto topBar = screen.removeFromTop (18.0f);
    g.setColour (colB);
    g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
    g.drawText ("MODE: " + routingControls.sysModeBox.getText().toUpperCase(),
                topBar, juce::Justification::centredLeft);

    g.setColour (colA);
    g.drawText (juce::String::formatted ("%.1f BPM", audioProcessor.getHostBpm()),
                topBar, juce::Justification::centredRight);

    g.setColour (colA.withAlpha (0.32f));
    g.drawHorizontalLine (juce::roundToInt (screen.getY() + 1.0f), screen.getX(), screen.getRight());
    screen.removeFromTop (4.0f);

    if (showLcdNotify)
    {
        auto titleArea = screen.removeFromTop (screen.getHeight() * 0.36f);
        g.setColour (CyberverbColours::textPrimary);
        g.setFont (juce::FontOptions (12.5f, juce::Font::bold));
        g.drawText (lcdNotifyTitle, titleArea, juce::Justification::centred, true);

        g.setColour (keypadActive ? colB : colA);
        g.setFont (juce::FontOptions (20.0f, juce::Font::bold));
        g.drawText (lcdNotifyValue, screen, juce::Justification::centred, true);
        return;
    }

    auto mainTitle = screen.removeFromTop (screen.getHeight() * 0.56f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (23.0f, juce::Font::bold));
    g.drawText (audioProcessor.getCurrentPresetName().toUpperCase(), mainTitle, juce::Justification::centred, true);

    const auto subInfo = spaceTabControls[0].revAlgoBox.getText() + "  +  " + spaceTabControls[1].revAlgoBox.getText();
    g.setColour (colB.withAlpha (0.90f));
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (subInfo, screen, juce::Justification::centred, true);
}

void CyberverbX1AudioProcessorEditor::drawFaderLabels (juce::Graphics& g, float w, float h)
{
    const char* labels[5] = { "DRY", "WET", "SPACE A", "MORPH", "SPACE B" };
    for (size_t i = 0; i < mainFaders.size(); ++i)
    {
        const auto fb = mainFaders[i].getBounds().toFloat();
        auto r = juce::Rectangle<float> (fb.getX() - 12.0f, fb.getBottom() + 2.0f, fb.getWidth() + 24.0f, 16.0f);
        g.setColour ((i == 0 || i == 2) ? CyberverbColours::engineA : CyberverbColours::engineB);
        g.setFont (juce::FontOptions (11.5f, juce::Font::bold));
        g.drawText (labels[i], r, juce::Justification::centred);
    }
    juce::ignoreUnused (w, h);
}

void CyberverbX1AudioProcessorEditor::drawSystemButtonLeds (juce::Graphics& g, float w, float h)
{
    const bool states[4] = {
        audioProcessor.apvts.getRawParameterValue ("sys_bypass")->load() > 0.5f,
        audioProcessor.isMixLockEnabled(),
        audioProcessor.isGlobalSyncEnabled(),
        false
    };

    const juce::Colour cols[4] = {
        juce::Colour::fromRGB (255, 65, 65), // Rot für BYPASS
        CyberverbColours::engineB,           // Gelb/Bernstein für MIX LOCK
        CyberverbColours::engineA,           // Cyan/Orange für SYNC
        CyberverbColours::engineA
    };

    const float r       = 0.0042f * w;
    const float yOffset = 0.0110f * h;

    for (size_t i = 0; i < 4; ++i)
    {
        const auto b   = systemButtons[i].getBounds().toFloat();
        const float cx = b.getCentreX();
        const float cy = b.getBottom() + yOffset;

        if (states[i])
        {
            g.setColour (cols[i].withAlpha (0.35f));
            g.fillEllipse (cx - r * 2.1f, cy - r * 2.1f, r * 4.2f, r * 4.2f);

            g.setColour (cols[i]);
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.fillEllipse (cx - r * 0.45f, cy - r * 0.45f, r * 0.7f, r * 0.7f);
        }
        else
        {
            // Deaktivierte LED als dunkler Sockel
            g.setColour (juce::Colour::fromRGB (14, 20, 28).withAlpha (0.85f));
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colour::fromRGB (45, 55, 70).withAlpha (0.6f));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
        }
    }
}

void CyberverbX1AudioProcessorEditor::drawMatrixLeds (juce::Graphics& g, float w, float h)
{
    constexpr float ledX[4] = { 0.4976f, 0.5369f, 0.5762f, 0.6173f };
    const float topLedY = 0.6140f * h;
    //const float botLedY = 0.8620f * h; // Direkt unterhalb der unteren Button-Reihe
    const float botLedY = matrixButtons[6].getBounds().toFloat().getBottom() + 0.0160f * h;
    const float r = 0.0042f * w;

    const bool aOn = audioProcessor.apvts.getRawParameterValue ("a_on")->load() > 0.5f;
    const bool bOn = audioProcessor.apvts.getRawParameterValue ("b_on")->load() > 0.5f;
    const bool frz = audioProcessor.apvts.getRawParameterValue ("sys_freeze")->load() > 0.5f;

    const bool topActive[4]    = { aOn, bOn, (aOn && ! bOn), (bOn && ! aOn) };
    const juce::Colour cols[4] = {
        CyberverbColours::engineA,
        CyberverbColours::engineB,
        CyberverbColours::engineA,
        CyberverbColours::engineB
    };

    auto drawSingleLed = [&] (float cx, float cy, bool isOn, juce::Colour col)
    {
        if (isOn)
        {
            g.setColour (col.withAlpha (0.35f));
            g.fillEllipse (cx - r * 2.1f, cy - r * 2.1f, r * 4.2f, r * 4.2f);

            g.setColour (col);
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);

            g.setColour (juce::Colours::white.withAlpha (0.75f));
            g.fillEllipse (cx - r * 0.45f, cy - r * 0.45f, r * 0.7f, r * 0.7f);
        }
        else
        {
            g.setColour (juce::Colour::fromRGB (14, 20, 28).withAlpha (0.85f));
            g.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
            g.setColour (juce::Colour::fromRGB (45, 55, 70).withAlpha (0.6f));
            g.drawEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f, 0.8f);
        }
    };

    // Obere 4 LEDs
    for (int i = 0; i < 4; ++i)
        drawSingleLed (ledX[i] * w, topLedY, topActive[i], cols[i]);

    // Untere LED nur unter dem FREEZE-Button (Spalte 2)
    drawSingleLed (ledX[2] * w, botLedY, frz, cols[2]);
}

void CyberverbX1AudioProcessorEditor::drawBottomStatusStrip (juce::Graphics& g, juce::Rectangle<float> stripBounds)
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    auto area = stripBounds.reduced (6.0f, 3.0f);
    const float totalW = area.getWidth();

    // Sektion 1: Routing & Engine Status
    auto sec1 = area.removeFromLeft (totalW * 0.24f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText ("MODE // " + routingControls.sysModeBox.getText().toUpperCase(),
                sec1.removeFromTop (sec1.getHeight() * 0.5f), juce::Justification::centredLeft);

    g.setColour (colB);
    g.drawText (audioProcessor.isMixLockEnabled() ? "MIX LOCK // ON" : "MIX LOCK // OFF",
                sec1, juce::Justification::centredLeft);

    // Sektion 2: Echtzeit-Oszilloskop / Morph-Vektor
    auto sec2 = area.removeFromLeft (totalW * 0.32f).reduced (6.0f, 0.0f);
    scopeClickArea = sec2;
    g.setColour (juce::Colour::fromRGB (8, 11, 16));
    g.fillRoundedRectangle (sec2, 3.0f);
    g.setColour (colA.withAlpha (0.30f));
    g.drawRoundedRectangle (sec2, 3.0f, 1.0f);

    if (bottomStripMode == BottomStripMode::Oscilloscope)
    {
        const auto inner = sec2.reduced (3.0f, 2.0f);
        juce::Path wave;
        for (int i = 0; i < CyberverbX1AudioProcessor::scopeBufferSize; ++i)
        {
            const float nx = static_cast<float> (i) / static_cast<float> (CyberverbX1AudioProcessor::scopeBufferSize - 1);
            const float sv = juce::jlimit (-1.0f, 1.0f, scopeCache[static_cast<size_t> (i)] * 1.4f);
            const float px = inner.getX() + nx * inner.getWidth();
            const float py = inner.getCentreY() - sv * (inner.getHeight() * 0.45f);
            if (i == 0) wave.startNewSubPath (px, py); else wave.lineTo (px, py);
        }
        g.setColour (colA);
        g.strokePath (wave, juce::PathStrokeType (1.4f));
    }
    else
    {
        const float m = audioProcessor.getLiveEffectiveMorph();
        auto bar = sec2.reduced (6.0f, 6.0f);
        const float split = bar.getX() + bar.getWidth() * (1.0f - m);
        g.setColour (colA.withAlpha (0.8f)); g.fillRoundedRectangle (bar.withRight (split), 2.0f);
        g.setColour (colB.withAlpha (0.8f)); g.fillRoundedRectangle (bar.withLeft (split), 2.0f);
    }

    // Sektion 3: Stereo Output Peak Meter
    auto sec3 = area.removeFromLeft (totalW * 0.26f).reduced (6.0f, 1.0f);
    auto drawMeter = [&] (juce::Rectangle<float> row, const char* lbl, float lvl, float hold)
    {
        g.setColour (CyberverbColours::textPrimary);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (lbl, row.removeFromLeft (14.0f), juce::Justification::centredLeft);
        auto mr = row.reduced (0.0f, 2.5f);
        g.setColour (juce::Colour::fromRGB (8, 11, 16));
        g.fillRoundedRectangle (mr, 2.0f);
        juce::ColourGradient grad (colA, mr.getX(), mr.getCentreY(), colB, mr.getRight(), mr.getCentreY(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (mr.withWidth (mr.getWidth() * juce::jlimit (0.0f, 1.0f, lvl)), 2.0f);
        if (hold > 0.01f)
        {
            g.setColour (juce::Colours::white);
            g.fillRect (mr.getX() + mr.getWidth() * juce::jlimit (0.0f, 1.0f, hold) - 1.0f, mr.getY(), 2.0f, mr.getHeight());
        }
    };
    drawMeter (sec3.removeFromTop (sec3.getHeight() * 0.5f), "L", smoothedLevelL, peakHoldL);
    drawMeter (sec3, "R", smoothedLevelR, peakHoldR);

    // Sektion 4: Sample Rate & DSP Online
    auto sec4 = area.withTrimmedLeft (6.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
    g.drawText (juce::String::formatted ("%.1f KHZ", audioProcessor.getCurrentSampleRateHz() / 1000.0),
                sec4.removeFromTop (sec4.getHeight() * 0.5f), juce::Justification::centredRight);
    g.setColour (colB);
    g.drawText ("DSP // ONLINE", sec4, juce::Justification::centredRight);
}

void CyberverbX1AudioProcessorEditor::drawRoutingPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds)
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (leftBounds, 6.0f);
    g.setColour (colA.withAlpha (0.45f));
    g.drawRoundedRectangle (leftBounds, 6.0f, 1.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("SIGNAL FLOW & CROSSOVER", leftBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, routingControls.splitFreqKnob, "SPLIT FREQ");
    drawKnobLabel (g, routingControls.driveKnob,     "IN DRIVE");
    drawKnobLabel (g, routingControls.crossFeedKnob, "CROSS FEED");

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (rightBounds, 6.0f);
    g.setColour (colB.withAlpha (0.45f));
    g.drawRoundedRectangle (rightBounds, 6.0f, 1.0f);
    g.setColour (colB);
    g.drawText ("DUCKING & GATED REVERB", rightBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, routingControls.duckAmtKnob, "DUCK AMT");
    drawKnobLabel (g, routingControls.duckRelKnob, "DUCK REL");
    drawKnobLabel (g, routingControls.gateThrKnob, "GATE THR");
    drawKnobLabel (g, routingControls.gateRelKnob, "GATE REL");
}

void CyberverbX1AudioProcessorEditor::drawSpacePanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds, int spaceIdx)
{
    const auto colA   = CyberverbColours::engineA;
    const auto accent = (spaceIdx == 0) ? colA : CyberverbColours::engineB;
    const auto& sc    = spaceTabControls[static_cast<size_t> (spaceIdx)];
    const juce::String tag = (spaceIdx == 0) ? "S1 // SPACE A " : "S2 // SPACE B ";

    // Linkes Panel: DELAY ENGINE
    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (leftBounds, 6.0f);
    g.setColour (colA.withAlpha (0.45f));
    g.drawRoundedRectangle (leftBounds, 6.0f, 1.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText (tag + "DELAY", leftBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    // Delay-Tap Visualizer
    auto tapRect = juce::Rectangle<float> (leftBounds.getX() + 10.0f, leftBounds.getY() + leftBounds.getHeight() * 0.30f,
                                           leftBounds.getWidth() - 20.0f, leftBounds.getHeight() * 0.25f);
    g.setColour (juce::Colour::fromRGB (8, 11, 16));
    g.fillRoundedRectangle (tapRect, 4.0f);
    const float fb = static_cast<float> (rightKnobs[static_cast<size_t> (8 + spaceIdx)].getValue());
    for (int t = 0; t < 8; ++t)
    {
        const float amp = std::pow (juce::jlimit (0.05f, 0.92f, fb), static_cast<float> (t));
        const float tx  = tapRect.getX() + 10.0f + (tapRect.getWidth() - 20.0f) * (static_cast<float> (t) / 7.5f);
        const float th  = (tapRect.getHeight() - 8.0f) * amp;
        g.setColour (colA.withAlpha (0.3f + 0.7f * amp));
        g.fillRoundedRectangle (tx, tapRect.getBottom() - 4.0f - th, 4.0f, th, 1.5f);
    }

    drawKnobLabel (g, sc.dlyTimeKnob,   "DLY TIME");
    drawKnobLabel (g, sc.dlyToneKnob,   "TONE");
    drawKnobLabel (g, sc.dlyRevMixKnob, "DLY > REV");

    // Rechtes Panel: REVERB ENGINE
    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (rightBounds, 6.0f);
    g.setColour (accent.withAlpha (0.45f));
    g.drawRoundedRectangle (rightBounds, 6.0f, 1.0f);
    g.setColour (accent);
    g.drawText (tag + "REVERB", rightBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    // Reverb Decay Curve Visualizer
    auto revRect = juce::Rectangle<float> (rightBounds.getX() + 10.0f, rightBounds.getY() + rightBounds.getHeight() * 0.30f,
                                           rightBounds.getWidth() - 20.0f, rightBounds.getHeight() * 0.25f);
    g.setColour (juce::Colour::fromRGB (8, 11, 16));
    g.fillRoundedRectangle (revRect, 4.0f);

    const float decayNorm = juce::jlimit (0.05f, 1.0f, static_cast<float> (rightKnobs[static_cast<size_t> (2 + spaceIdx)].getValue()) / 15.0f);
    juce::Path env;
    env.startNewSubPath (revRect.getX() + 6.0f, revRect.getBottom() - 4.0f);
    env.lineTo (revRect.getX() + 18.0f, revRect.getY() + 6.0f);
    for (int x = 1; x <= 32; ++x)
    {
        const float nx = static_cast<float> (x) / 32.0f;
        const float yVal = std::exp (-nx * (4.5f - decayNorm * 3.6f));
        env.lineTo (revRect.getX() + 18.0f + nx * (revRect.getWidth() - 24.0f),
                    revRect.getBottom() - 4.0f - yVal * (revRect.getHeight() - 10.0f));
    }
    g.setColour (accent);
    g.strokePath (env, juce::PathStrokeType (1.8f));

    drawKnobLabel (g, sc.preDlyKnob,   "PRE-DELAY");
    drawKnobLabel (g, sc.diffuseKnob,  spaceIdx == 0 ? "DIFFUSION" : "SHIMMER");
    drawKnobLabel (g, sc.modDepthKnob, "MOD DEPTH");
}

void CyberverbX1AudioProcessorEditor::drawModulationPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds)
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (leftBounds, 6.0f);
    g.setColour (colA.withAlpha (0.45f));
    g.drawRoundedRectangle (leftBounds, 6.0f, 1.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("DUAL LFO GENERATORS", leftBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, modDynControls.lfo1RateKnob, "LFO 1 RATE");
    drawKnobLabel (g, modDynControls.lfo2RateKnob, "LFO 2 RATE");

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (rightBounds, 6.0f);
    g.setColour (colB.withAlpha (0.45f));
    g.drawRoundedRectangle (rightBounds, 6.0f, 1.0f);
    g.setColour (colB);
    g.drawText ("INPUT ENVELOPE FOLLOWER", rightBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, modDynControls.envSensKnob, "SENS");
    drawKnobLabel (g, modDynControls.envAtkKnob,  "ATTACK");
    drawKnobLabel (g, modDynControls.envRelKnob,  "RELEASE");
    drawKnobLabel (g, modDynControls.envBiasKnob, "BIAS");
}

void CyberverbX1AudioProcessorEditor::drawEqPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds)
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (leftBounds, 6.0f);
    g.setColour (colA.withAlpha (0.45f));
    g.drawRoundedRectangle (leftBounds, 6.0f, 1.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("WET EQ // LOW & LOW-MID", leftBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, eqControls.lowFreqKnob,  "LOW FREQ");
    drawKnobLabel (g, eqControls.lowGainKnob,  "LOW GAIN");
    drawKnobLabel (g, eqControls.mid1FreqKnob, "MID 1 FREQ");
    drawKnobLabel (g, eqControls.mid1GainKnob, "MID 1 GAIN");

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (rightBounds, 6.0f);
    g.setColour (colB.withAlpha (0.45f));
    g.drawRoundedRectangle (rightBounds, 6.0f, 1.0f);
    g.setColour (colB);
    g.drawText ("WET EQ // HIGH-MID & HIGH", rightBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    drawKnobLabel (g, eqControls.mid2FreqKnob, "MID 2 FREQ");
    drawKnobLabel (g, eqControls.mid2GainKnob, "MID 2 GAIN");
    drawKnobLabel (g, eqControls.highFreqKnob, "HIGH FREQ");
    drawKnobLabel (g, eqControls.highGainKnob, "HIGH GAIN");
}

void CyberverbX1AudioProcessorEditor::drawMatrixPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds)
{
    const auto colA = CyberverbColours::engineA;
    const auto colB = CyberverbColours::engineB;

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (leftBounds, 6.0f);
    g.setColour (colA.withAlpha (0.45f));
    g.drawRoundedRectangle (leftBounds, 6.0f, 1.0f);
    g.setColour (colA);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("MODULATION MATRIX (SLOTS 1-4)", leftBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    g.setColour (CyberverbColours::panelDark.withAlpha (0.78f));
    g.fillRoundedRectangle (rightBounds, 6.0f);
    g.setColour (colB.withAlpha (0.45f));
    g.drawRoundedRectangle (rightBounds, 6.0f, 1.0f);
    g.setColour (colB);
    g.drawText ("LIVE MODULATION MONITOR", rightBounds.reduced (10.0f, 4.0f).removeFromTop (19.0f), juce::Justification::centredLeft);

    auto drawBar = [&] (float yOffset, const juce::String& name, float val, juce::Colour c)
    {
        auto r = juce::Rectangle<float> (rightBounds.getX() + 12.0f, rightBounds.getY() + yOffset, rightBounds.getWidth() - 24.0f, 18.0f);
        g.setColour (juce::Colour::fromRGB (8, 11, 16));
        g.fillRoundedRectangle (r, 3.0f);
        const float mid = r.getCentreX();
        const float bw = std::abs (juce::jlimit (-1.0f, 1.0f, val)) * (r.getWidth() * 0.45f);
        g.setColour (c.withAlpha (0.85f));
        g.fillRoundedRectangle (val >= 0.0f ? mid : mid - bw, r.getY() + 2.0f, juce::jmax (2.0f, bw), r.getHeight() - 4.0f, 2.0f);
        g.setColour (CyberverbColours::textPrimary);
        g.setFont (juce::FontOptions (10.5f, juce::Font::bold));
        g.drawText (name, r.reduced (6.0f, 0.0f), juce::Justification::centredLeft);
    };

    drawBar (38.0f,  "LFO 1",        audioProcessor.getLiveLfo1Value(), colA);
    drawBar (66.0f,  "LFO 2",        audioProcessor.getLiveLfo2Value(), colB);
    drawBar (94.0f,  "ENV FOLLOWER", audioProcessor.getLiveEnvFollower(), colA);
}

void CyberverbX1AudioProcessorEditor::drawAssignMidiLearnBadges (juce::Graphics& g)
{
    const int learningSlot = audioProcessor.getMidiLearnTargetSlot();
    for (int i = 0; i < CyberverbX1AudioProcessor::numAssignKnobs; ++i)
    {
        const auto kb = rightKnobs[static_cast<size_t> (i + 2)].getBounds().toFloat();
        const int cc = audioProcessor.getAssignedMidiCC (i);

        if (learningSlot == i)
        {
            g.setColour (CyberverbColours::engineB);
            g.drawEllipse (kb.reduced (1.0f), 2.0f);
        }
        else if (cc >= 0)
        {
            auto badge = juce::Rectangle<float> (kb.getCentreX() - 20.0f, kb.getBottom() - 11.0f, 40.0f, 11.0f);
            g.setColour (CyberverbColours::panelDark.withAlpha (0.9f));
            g.fillRoundedRectangle (badge, 2.5f);
            g.setColour (CyberverbColours::engineA);
            g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
            g.drawText ("CC " + juce::String (cc), badge, juce::Justification::centred);
        }
    }
}