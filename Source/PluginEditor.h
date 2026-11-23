#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "UI/CyberverbLookAndFeel.h"
#include "UI/CustomControls.h"
#include "BinaryData.h"

class CyberverbX1AudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    enum class ActiveTab
    {
        Routing = 0,
        SpaceA,
        SpaceB,
        Modulation,
        EQ,
        Matrix,
        NumTabs
    };

    enum class BottomStripMode
    {
        Oscilloscope = 0,
        MorphVector,
        NumModes
    };

    explicit CyberverbX1AudioProcessorEditor (CyberverbX1AudioProcessor&);
    ~CyberverbX1AudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    bool keyPressed (const juce::KeyPress& key) override;

    void triggerLcdNotification (const juce::String& title, const juce::String& value);

private:
    struct RoutingControls
    {
        juce::ComboBox sysModeBox;
        juce::Slider splitFreqKnob, driveKnob, crossFeedKnob;
        juce::Slider duckAmtKnob, duckRelKnob, gateThrKnob, gateRelKnob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sysModeAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> splitFreqAtt, driveAtt, crossFeedAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> duckAmtAtt, duckRelAtt, gateThrAtt, gateRelAtt;
    };

    struct SpaceTabControls
    {
        juce::ComboBox dlyModeBox, dlySyncBox, revAlgoBox;
        juce::Slider dlyTimeKnob, dlyToneKnob, dlyRevMixKnob;
        juce::Slider preDlyKnob, diffuseKnob, modDepthKnob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> dlyModeAtt, dlySyncAtt, revAlgoAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dlyTimeAtt, dlyToneAtt, dlyRevMixAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preDlyAtt, diffuseAtt, modDepthAtt;
    };

    struct ModDynControls
    {
        juce::ComboBox lfo1ShapeBox, lfo2ShapeBox;
        juce::Slider lfo1RateKnob, lfo2RateKnob;
        juce::Slider envSensKnob, envAtkKnob, envRelKnob, envBiasKnob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> lfo1ShapeAtt, lfo2ShapeAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lfo1RateAtt, lfo2RateAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envSensAtt, envAtkAtt, envRelAtt, envBiasAtt;
    };

    struct EqControls
    {
        juce::Slider lowFreqKnob, lowGainKnob, mid1FreqKnob, mid1GainKnob;
        juce::Slider mid2FreqKnob, mid2GainKnob, highFreqKnob, highGainKnob;

        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowFreqAtt, lowGainAtt, mid1FreqAtt, mid1GainAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mid2FreqAtt, mid2GainAtt, highFreqAtt, highGainAtt;
    };

    struct ModSlotControls
    {
        juce::ComboBox srcBox, dstBox;
        juce::Slider amtKnob;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> srcAtt, dstAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> amtAtt;
    };

    void timerCallback() override;

    void showAssignKnobMidiLearnMenu (int assignIndex);
    void updateAssignKnobTooltip (int assignIndex);
    void handleSaveMidiMappingClick();
    void handleLoadMidiMappingClick();

    void setKeypadTarget (const juce::String& paramID, const juce::String& displayName);
    void handleKeypadDigit (int digit);
    void handleKeypadClear();
    void handleKeypadEnter();
    void handleKeypadToggleSign();

    void refreshPresetList();
    void handleSavePresetClick (bool forceSaveAsDialog);
    void handleLoadPresetClick();
    void handleRenameOrMovePresetClick();
    void handleDeletePresetClick();
    void handleRestoreFactoryPresetsClick();
    void handleFlushClick();
    void stepPreset (int delta);

    void initTabControls();
    void setActiveTab (ActiveTab newTab);
    void updateTabVisibility();
    void handleMatrixButton (int buttonIndex);
    void handleSystemButton (int buttonIndex);
    void showSettingsMenu();

    void drawLcdScreen (juce::Graphics& g, juce::Rectangle<float> lcdBounds);
    void drawBottomStatusStrip (juce::Graphics& g, juce::Rectangle<float> stripBounds);
    void drawMatrixLeds (juce::Graphics& g, float w, float h);
    void drawSystemButtonLeds (juce::Graphics& g, float w, float h);
    void drawFaderLabels (juce::Graphics& g, float w, float h);
    void drawAssignMidiLearnBadges (juce::Graphics& g);

    void drawRoutingPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds);
    void drawSpacePanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds, int spaceIdx);
    void drawModulationPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds);
    void drawEqPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds);
    void drawMatrixPanels (juce::Graphics& g, juce::Rectangle<float> leftBounds, juce::Rectangle<float> rightBounds);

    CyberverbX1AudioProcessor& audioProcessor;
    CyberverbLookAndFeel cyberverbLookAndFeel;

    juce::Image backgroundImage;
    std::array<juce::Image, static_cast<size_t> (ActiveTab::NumTabs)> tabImages;
    std::array<juce::TextButton, static_cast<size_t> (ActiveTab::NumTabs)> tabButtons;
    juce::TextButton settingsButton;

    juce::ComboBox presetBox;
    TopBarPresetButton savePresetButton;
    TopBarPresetButton loadPresetButton;
    TopBarPresetButton flushButton;
    juce::TextButton prevPresetButton;
    juce::TextButton nextPresetButton;
    std::unique_ptr<juce::FileChooser> activeFileChooser;
    bool suppressPresetBoxCallback = false;
    juce::Array<CyberverbX1AudioProcessor::PresetEntry> cachedPresetEntries;

    std::array<juce::TextButton, 12> keypadButtons;
    juce::String keypadTargetParamID { "out_gain" };
    juce::String keypadTargetLabel   { "OUTPUT GAIN" };
    juce::String keypadBuffer;
    bool keypadNegative = false;
    bool keypadActive   = false;

    // 8 Matrix-Buttons (ON A, ON B, SOLO A, SOLO B, COPY A>B, SWAP A/B, FREEZE, RAND)
    std::array<juce::TextButton, 8> matrixButtons;
    // 4 System-Buttons links (BYPASS, MIX LOCK, SYNC, SYSTEM)
    std::array<juce::TextButton, 4> systemButtons;

    // 10 rechte Drehregler: [0] = OUTPUT, [1] = WIDTH, [2..9] = DECAY A/B, SIZE A/B, DAMP A/B, DLY FEED A/B
    std::array<AssignableKnobSlider, 10> rightKnobs;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 10> rightKnobAttachments;

    // 5 vertikale Fader unten links: DRY, WET, SPACE A, MORPH, SPACE B
    std::array<juce::Slider, 5> mainFaders;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 5> mainFaderAttachments;

    RoutingControls routingControls;
    std::array<SpaceTabControls, 2> spaceTabControls;
    ModDynControls modDynControls;
    EqControls eqControls;
    std::array<ModSlotControls, 4> modMatrixSlots;

    ActiveTab currentTab = ActiveTab::SpaceA;

    juce::String lcdNotifyTitle;
    juce::String lcdNotifyValue;
    bool showLcdNotify = false;
    int lcdNotifyTicks = 0;

    BottomStripMode bottomStripMode = BottomStripMode::Oscilloscope;
    juce::Rectangle<float> bottomStripBounds;
    juce::Rectangle<float> scopeClickArea;
    std::array<float, CyberverbX1AudioProcessor::scopeBufferSize> scopeCache {};
    float smoothedLevelL   = 0.0f;
    float smoothedLevelR   = 0.0f;
    float peakHoldL        = 0.0f;
    float peakHoldR        = 0.0f;
    int peakHoldTicksL     = 0;
    int peakHoldTicksR     = 0;
    int midiLedHoldTicks   = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CyberverbX1AudioProcessorEditor)
};