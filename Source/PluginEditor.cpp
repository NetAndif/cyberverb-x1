#include "PluginProcessor.h"
#include "PluginEditor.h"

CyberverbX1AudioProcessorEditor::CyberverbX1AudioProcessorEditor (CyberverbX1AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&cyberverbLookAndFeel);
    setWantsKeyboardFocus (true);
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::background_main_png, BinaryData::background_main_pngSize);
    backgroundImage = juce::ImageCache::getFromMemory (BinaryData::background_main_png, BinaryData::background_main_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::Routing)]    = juce::ImageCache::getFromMemory (BinaryData::tab_routing_png,    BinaryData::tab_routing_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::SpaceA)]     = juce::ImageCache::getFromMemory (BinaryData::tab_space_a_png,    BinaryData::tab_space_a_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::SpaceB)]     = juce::ImageCache::getFromMemory (BinaryData::tab_space_b_png,    BinaryData::tab_space_b_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::Modulation)] = juce::ImageCache::getFromMemory (BinaryData::tab_modulation_png, BinaryData::tab_modulation_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::EQ)]         = juce::ImageCache::getFromMemory (BinaryData::tab_eq_png,         BinaryData::tab_eq_pngSize);
    tabImages[static_cast<size_t> (ActiveTab::Matrix)]     = juce::ImageCache::getFromMemory (BinaryData::tab_matrix_png,     BinaryData::tab_matrix_pngSize);

    // --- Top-Bar Preset-Verwaltung ---
    presetBox.setJustificationType (juce::Justification::centred);
    presetBox.setTooltip ("Select Preset (Factory / User)");
    presetBox.onChange = [this]
    {
        if (suppressPresetBoxCallback)
            return;

        const int entryIndex = presetBox.getSelectedId() - 1;
        if (juce::isPositiveAndBelow (entryIndex, cachedPresetEntries.size()))
        {
            const auto& entry = cachedPresetEntries.getReference (entryIndex);
            if (audioProcessor.loadPresetEntry (entry))
                triggerLcdNotification (entry.category.isEmpty() ? "INIT" : entry.category.toUpperCase(), entry.name);
        }
    };
    addAndMakeVisible (presetBox);

    savePresetButton.setAlpha (0.0f);
    savePresetButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    savePresetButton.setTooltip ("Save Preset (Right-Click: Save As...)");
    savePresetButton.onClick = [this] { handleSavePresetClick (false); };
    savePresetButton.onRightClick = [this] { handleSavePresetClick (true); };
    addAndMakeVisible (savePresetButton);

    loadPresetButton.setAlpha (0.0f);
    loadPresetButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    loadPresetButton.setTooltip ("Import External Preset (.xml) (Right-Click: Manager)");
    loadPresetButton.onClick = [this] { handleLoadPresetClick(); };
    loadPresetButton.onRightClick = [this]
    {
        juce::PopupMenu mgr;
        mgr.addSectionHeader ("PRESET MANAGER");
        mgr.addItem (1, "Import External Preset (.xml)...");
        mgr.addItem (2, "Save As / Set Category...");
        mgr.addItem (3, "Rename / Move Current User Preset...",
                     audioProcessor.isCurrentPresetSaved() && ! audioProcessor.isCurrentPresetFactory());
        mgr.addItem (4, "Delete Current User Preset...",
                     audioProcessor.isCurrentPresetSaved() && ! audioProcessor.isCurrentPresetFactory());
        mgr.addSeparator();
        mgr.addItem (5, "Restore Factory Presets");
        mgr.addItem (6, "Open Presets Folder...");

        mgr.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&loadPresetButton), [this] (int res)
        {
            switch (res)
            {
                case 1: handleLoadPresetClick(); break;
                case 2: handleSavePresetClick (true); break;
                case 3: handleRenameOrMovePresetClick(); break;
                case 4: handleDeletePresetClick(); break;
                case 5: handleRestoreFactoryPresetsClick(); break;
                case 6: audioProcessor.getPresetDirectory().revealToUser(); break;
                default: break;
            }
        });
    };
    addAndMakeVisible (loadPresetButton);

    flushButton.setAlpha (0.0f);
    flushButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    flushButton.setTooltip ("FLUSH: Clear All Delay & Reverb Buffers");
    flushButton.onClick = [this] { handleFlushClick(); };
    addAndMakeVisible (flushButton);

    prevPresetButton.setAlpha (0.0f);
    prevPresetButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    prevPresetButton.onClick = [this] { stepPreset (-1); };
    addAndMakeVisible (prevPresetButton);

    nextPresetButton.setAlpha (0.0f);
    nextPresetButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    nextPresetButton.onClick = [this] { stepPreset (1); };
    addAndMakeVisible (nextPresetButton);

    refreshPresetList();

    // --- 10er-Nummernblock ---
    for (size_t i = 0; i < keypadButtons.size(); ++i)
    {
        auto& btn = keypadButtons[i];
        btn.setAlpha (0.0f);
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);

        if (i < 9)       btn.onClick = [this, d = static_cast<int> (i + 1)] { handleKeypadDigit (d); };
        else if (i == 9) btn.onClick = [this] { handleKeypadClear(); };
        else if (i == 10) btn.onClick = [this] { handleKeypadDigit (0); };
        else             btn.onClick = [this] { handleKeypadEnter(); };

        addAndMakeVisible (btn);
    }

    // --- 6 Center-Tabs ---
    for (size_t i = 0; i < tabButtons.size(); ++i)
    {
        auto& btn = tabButtons[i];
        btn.setAlpha (0.0f);
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        btn.onClick = [this, i] { setActiveTab (static_cast<ActiveTab> (i)); };
        addAndMakeVisible (btn);
    }

    // --- 8 Matrix-Buttons (ON A, ON B, SOLO A, SOLO B, COPY A>B, SWAP A/B, FREEZE, RAND) ---
    for (size_t i = 0; i < matrixButtons.size(); ++i)
    {
        auto& btn = matrixButtons[i];
        btn.setAlpha (0.0f);
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        btn.onClick = [this, i] { handleMatrixButton (static_cast<int> (i)); };
        addAndMakeVisible (btn);
    }

    // --- 4 System-Buttons links (BYPASS, MIX LOCK, SYNC, SYSTEM) ---
    for (size_t i = 0; i < systemButtons.size(); ++i)
    {
        auto& btn = systemButtons[i];
        btn.setAlpha (0.0f);
        btn.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        btn.onClick = [this, i] { handleSystemButton (static_cast<int> (i)); };
        addAndMakeVisible (btn);
    }

    settingsButton.setAlpha (0.0f);
    settingsButton.setMouseCursor (juce::MouseCursor::PointingHandCursor);
    settingsButton.onClick = [this] { showSettingsMenu(); };
    addAndMakeVisible (settingsButton);

    // --- 10 Rechte Drehregler (OUTPUT, WIDTH + DECAY/SIZE/DAMP/DLY FEED für A & B) ---
    const char* rightParamIDs[10] = {
        "out_gain", "out_width",
        "a_rev_decay", "b_rev_decay",
        "a_rev_size",  "b_rev_size",
        "a_rev_damp",  "b_rev_damp",
        "a_dly_feed",  "b_dly_feed"
    };
    const char* rightLabels[10] = {
        "OUTPUT GAIN", "STEREO WIDTH",
        "SPACE A // DECAY", "SPACE B // DECAY",
        "SPACE A // SIZE",  "SPACE B // SIZE",
        "SPACE A // DAMP",  "SPACE B // DAMP",
        "SPACE A // DLY FEED", "SPACE B // DLY FEED"
    };

    for (size_t i = 0; i < rightKnobs.size(); ++i)
    {
        auto& knob = rightKnobs[i];
        knob.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        knob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);

        const auto accent = (i % 2 == 0) ? CyberverbColours::engineA : CyberverbColours::engineB;
        knob.setColour (juce::Slider::rotarySliderFillColourId, accent);

        const juce::String pID (rightParamIDs[i]);
        const juce::String lbl (rightLabels[i]);

        if (auto* param = audioProcessor.apvts.getParameter (pID))
            knob.setDoubleClickReturnValue (true, param->getDefaultValue());

        knob.onValueChange = [this, i, pID, lbl]
        {
            const double v = rightKnobs[i].getValue();
            setKeypadTarget (pID, lbl);

            if (i == 0)
                triggerLcdNotification (lbl, juce::String::formatted ("%+.1f DB", v));
            else if (i == 1)
                triggerLcdNotification (lbl, juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)));
            else if (i == 2 || i == 3)
                triggerLcdNotification (lbl, juce::String::formatted ("%.2f S", v));
            else
                triggerLcdNotification (lbl, juce::String::formatted ("%d%%", juce::roundToInt (v * 100.0)));
        };

        addAndMakeVisible (knob);
        rightKnobAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.apvts, pID, knob);

        if (i >= 2)
        {
            const int assignIdx = static_cast<int> (i - 2);
            knob.onRightClick = [this, assignIdx] { showAssignKnobMidiLearnMenu (assignIdx); };
            updateAssignKnobTooltip (assignIdx);
        }
    }

    // --- 5 Fader unten links: DRY, WET, SPACE A, MORPH, SPACE B ---
    const char* faderIDs[5]    = { "mix_dry", "mix_wet", "space_a_lvl", "space_morph", "space_b_lvl" };
    const char* faderLabels[5] = { "DRY LEVEL", "WET LEVEL", "SPACE A LEVEL", "SPACE MORPH", "SPACE B LEVEL" };
    const juce::Colour faderColours[5] = {
        CyberverbColours::engineA,
        CyberverbColours::engineB,
        CyberverbColours::engineA,
        CyberverbColours::engineB,
        CyberverbColours::engineB
    };

    for (size_t i = 0; i < mainFaders.size(); ++i)
    {
        auto& fader = mainFaders[i];
        fader.setSliderStyle (juce::Slider::LinearVertical);
        fader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        fader.setColour (juce::Slider::thumbColourId, faderColours[i]);

        const juce::String pID (faderIDs[i]);
        const juce::String lbl (faderLabels[i]);

        if (auto* param = audioProcessor.apvts.getParameter (pID))
            fader.setDoubleClickReturnValue (true, param->getDefaultValue());

        fader.onValueChange = [this, i, pID, lbl]
        {
            setKeypadTarget (pID, lbl);
            triggerLcdNotification (lbl, juce::String::formatted ("%d%%", juce::roundToInt (mainFaders[i].getValue() * 100.0)));
        };

        addAndMakeVisible (fader);
        mainFaderAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
            audioProcessor.apvts, pID, fader);
    }

    initTabControls();
    startTimerHz (30);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio (16.0 / 9.0);
    setResizeLimits (800, 450, 2200, 1238);

   #if JUCE_LINUX
    setSize (1400, 788);
   #else
    setSize (1120, 630);
   #endif
}

CyberverbX1AudioProcessorEditor::~CyberverbX1AudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ==============================================================================
// System- & Matrix-Buttons
// ==============================================================================
void CyberverbX1AudioProcessorEditor::handleSystemButton (int buttonIndex)
{
    switch (buttonIndex)
    {
        case 0: // BYPASS
            if (auto* p = audioProcessor.apvts.getParameter ("sys_bypass"))
            {
                const bool next = p->getValue() < 0.5f;
                p->setValueNotifyingHost (next ? 1.0f : 0.0f);
                triggerLcdNotification ("SYSTEM BYPASS", next ? "BYPASSED" : "ACTIVE");
            }
            break;

        case 1: // MIX LOCK
        {
            const bool next = ! audioProcessor.isMixLockEnabled();
            audioProcessor.setMixLockEnabled (next);
            triggerLcdNotification ("DRY / WET MIX LOCK", next ? "LOCKED (SEND MODE)" : "UNLOCKED");
            break;
        }

        case 2: // SYNC
        {
            const bool next = ! audioProcessor.isGlobalSyncEnabled();
            audioProcessor.setGlobalSyncEnabled (next);
            triggerLcdNotification ("TEMPO SYNC", next ? "BPM SYNCED" : "FREE (MS)");
            break;
        }

        case 3: // SYSTEM
            showSettingsMenu();
            break;

        default:
            break;
    }
    repaint();
}

void CyberverbX1AudioProcessorEditor::handleMatrixButton (int buttonIndex)
{
    auto toggleBoolParam = [this] (const juce::String& id, const juce::String& title)
    {
        if (auto* p = audioProcessor.apvts.getParameter (id))
        {
            const bool next = p->getValue() < 0.5f;
            p->setValueNotifyingHost (next ? 1.0f : 0.0f);
            triggerLcdNotification (title, next ? "ENABLED" : "MUTED");
        }
    };

    switch (buttonIndex)
    {
        case 0: toggleBoolParam ("a_on", "SPACE ENGINE A"); break;
        case 1: toggleBoolParam ("b_on", "SPACE ENGINE B"); break;
        case 2: // SOLO A
            if (auto* pA = audioProcessor.apvts.getParameter ("a_on"))
                if (auto* pB = audioProcessor.apvts.getParameter ("b_on"))
                {
                    const bool isAlreadySoloA = (pA->getValue() > 0.5f && pB->getValue() < 0.5f);
                    pA->setValueNotifyingHost (1.0f);
                    pB->setValueNotifyingHost (isAlreadySoloA ? 1.0f : 0.0f);
                    triggerLcdNotification ("SOLO A", isAlreadySoloA ? "OFF (A+B ACTIVE)" : "ACTIVE");
                }
            break;
        case 3: // SOLO B
            if (auto* pA = audioProcessor.apvts.getParameter ("a_on"))
                if (auto* pB = audioProcessor.apvts.getParameter ("b_on"))
                {
                    const bool isAlreadySoloB = (pB->getValue() > 0.5f && pA->getValue() < 0.5f);
                    pB->setValueNotifyingHost (1.0f);
                    pA->setValueNotifyingHost (isAlreadySoloB ? 1.0f : 0.0f);
                    triggerLcdNotification ("SOLO B", isAlreadySoloB ? "OFF (A+B ACTIVE)" : "ACTIVE");
                }
            break;
        case 4: // COPY A>B
            audioProcessor.copySpaceParameters (1, 2);
            triggerLcdNotification ("SPACE COPY", "A -> B COPIED");
            break;
        case 5: // SWAP A/B
            audioProcessor.swapSpaceParameters();
            triggerLcdNotification ("SPACE SWAP", "A <-> B SWAPPED");
            break;
        case 6: // FREEZE
            toggleBoolParam ("sys_freeze", "INFINITE FREEZE");
            break;
        case 7: // RAND
        {
            const int target = (currentTab == ActiveTab::SpaceB) ? 2 : 1;
            audioProcessor.randomizeSpaceParameters (target);
            triggerLcdNotification (target == 1 ? "RANDOMIZE A" : "RANDOMIZE B", "NEW SPACE GENERATED");
            break;
        }
        default: break;
    }
    repaint();
}

// ==============================================================================
// MIDI Learn & Keypad
// ==============================================================================
void CyberverbX1AudioProcessorEditor::updateAssignKnobTooltip (int assignIndex)
{
    auto& knob = rightKnobs[static_cast<size_t> (assignIndex + 2)];
    const int cc = audioProcessor.getAssignedMidiCC (assignIndex);
    knob.setTooltip (cc >= 0 ? ("MIDI CC " + juce::String (cc) + " (Right-Click: MIDI Learn)")
                             : "Right-Click: MIDI Learn");
}

void CyberverbX1AudioProcessorEditor::showAssignKnobMidiLearnMenu (int assignIndex)
{
    const int currentCC = audioProcessor.getAssignedMidiCC (assignIndex);
    const bool isLearning = (audioProcessor.getMidiLearnTargetSlot() == assignIndex);
    const auto currentMode = audioProcessor.getAssignMidiCCMode (assignIndex);

    juce::PopupMenu menu;
    menu.addSectionHeader ("SLOT " + juce::String (assignIndex + 1) + " // MIDI LEARN");

    juce::PopupMenu modeMenu;
    modeMenu.addItem (10, "Absolute (0-127)", true, currentMode == CyberverbX1AudioProcessor::MidiCCMode::Absolute);
    modeMenu.addItem (11, "Relative (2's Comp)", true, currentMode == CyberverbX1AudioProcessor::MidiCCMode::RelativeTwosComp);
    modeMenu.addItem (12, "Relative (Bin Offset)", true, currentMode == CyberverbX1AudioProcessor::MidiCCMode::RelativeBinOffset);
    menu.addSubMenu ("CC Mode", modeMenu);

    menu.addItem (isLearning ? 2 : 1, isLearning ? "Cancel MIDI Learn" : "MIDI Learn (Move Hardware Controller...)");
    menu.addItem (3, currentCC >= 0 ? ("Clear MIDI CC (" + juce::String (currentCC) + ")") : "Clear MIDI CC", currentCC >= 0);

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&rightKnobs[static_cast<size_t> (assignIndex + 2)]),
                        [this, assignIndex] (int res)
    {
        if (res == 1)      { audioProcessor.startMidiLearnForAssign (assignIndex); triggerLcdNotification ("MIDI LEARN", "MOVE CC..."); }
        else if (res == 2) { audioProcessor.cancelMidiLearn(); triggerLcdNotification ("MIDI LEARN", "CANCELLED"); }
        else if (res == 3) { audioProcessor.clearMidiLearnForAssign (assignIndex); updateAssignKnobTooltip (assignIndex); }
        else if (res >= 10 && res <= 12)
        {
            audioProcessor.setAssignMidiCCMode (assignIndex, static_cast<CyberverbX1AudioProcessor::MidiCCMode> (res - 10));
            updateAssignKnobTooltip (assignIndex);
        }
        repaint();
    });
}

void CyberverbX1AudioProcessorEditor::handleSaveMidiMappingClick()
{
    activeFileChooser = std::make_unique<juce::FileChooser> ("Save MIDI Map", audioProcessor.getMidiMapDirectory(), "*.xml");
    activeFileChooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
                                    [this] (const juce::FileChooser& c)
    {
        auto f = c.getResult();
        if (f != juce::File() && audioProcessor.saveMidiMappingToFile (f.withFileExtension (".xml")))
            triggerLcdNotification ("MIDI MAP SAVED", f.getFileNameWithoutExtension().toUpperCase());
    });
}

void CyberverbX1AudioProcessorEditor::handleLoadMidiMappingClick()
{
    activeFileChooser = std::make_unique<juce::FileChooser> ("Load MIDI Map", audioProcessor.getMidiMapDirectory(), "*.xml");
    activeFileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                    [this] (const juce::FileChooser& c)
    {
        if (audioProcessor.loadMidiMappingFromFile (c.getResult()))
        {
            for (int k = 0; k < CyberverbX1AudioProcessor::numAssignKnobs; ++k) updateAssignKnobTooltip (k);
            triggerLcdNotification ("MIDI MAP LOADED", "SUCCESS");
            repaint();
        }
    });
}

void CyberverbX1AudioProcessorEditor::setKeypadTarget (const juce::String& paramID, const juce::String& displayName)
{
    if (keypadTargetParamID != paramID)
    {
        keypadBuffer.clear();
        keypadNegative = false;
        keypadActive   = false;
    }
    keypadTargetParamID = paramID;
    keypadTargetLabel   = displayName;
}

void CyberverbX1AudioProcessorEditor::handleKeypadDigit (int digit)
{
    if (keypadBuffer.length() >= 6) return;
    keypadBuffer = (keypadBuffer == "0") ? juce::String (digit) : (keypadBuffer + juce::String (digit));
    keypadActive = true;
    triggerLcdNotification ("KEYPAD // " + keypadTargetLabel, (keypadNegative ? "-" : "") + keypadBuffer + "_");
}

void CyberverbX1AudioProcessorEditor::handleKeypadClear()
{
    if (keypadBuffer.isEmpty()) { handleKeypadToggleSign(); return; }
    keypadBuffer.clear(); keypadNegative = false; keypadActive = false;
    triggerLcdNotification ("KEYPAD // " + keypadTargetLabel, "CLEARED");
}

void CyberverbX1AudioProcessorEditor::handleKeypadToggleSign()
{
    keypadNegative = ! keypadNegative;
    keypadActive = true;
    triggerLcdNotification ("KEYPAD // " + keypadTargetLabel, (keypadNegative ? "-" : "+") + keypadBuffer + "_");
}

void CyberverbX1AudioProcessorEditor::handleKeypadEnter()
{
    if (keypadBuffer.isEmpty()) return;
    if (auto* param = audioProcessor.apvts.getParameter (keypadTargetParamID))
    {
        const double raw = keypadBuffer.getDoubleValue() * (keypadNegative ? -1.0 : 1.0);
        float val = static_cast<float> (raw);
        if (keypadTargetParamID.endsWith ("_lvl") || keypadTargetParamID.startsWith ("mix_")
            || keypadTargetParamID.endsWith ("_size") || keypadTargetParamID.endsWith ("_damp")
            || keypadTargetParamID.endsWith ("_feed") || keypadTargetParamID == "out_width")
        {
            val = static_cast<float> (std::abs (raw)) / 100.0f;
        }
        param->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, param->convertTo0to1 (val)));
    }
    keypadBuffer.clear(); keypadNegative = false; keypadActive = false;
}

bool CyberverbX1AudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    const auto ch = key.getTextCharacter();
    if (ch >= '0' && ch <= '9') { handleKeypadDigit (ch - '0'); return true; }
    if (ch == '-')              { handleKeypadToggleSign(); return true; }
    if (key == juce::KeyPress::returnKey) { handleKeypadEnter(); return true; }
    if (key == juce::KeyPress::backspaceKey || key == juce::KeyPress::escapeKey) { handleKeypadClear(); return true; }
    return false;
}

// ==============================================================================
// Presets & Timer
// ==============================================================================
void CyberverbX1AudioProcessorEditor::refreshPresetList()
{
    suppressPresetBoxCallback = true;
    presetBox.clear (juce::dontSendNotification);
    cachedPresetEntries = audioProcessor.getAllPresetEntries();

    int matchId = 1;
    for (int i = 0; i < cachedPresetEntries.size(); ++i)
    {
        const auto& e = cachedPresetEntries.getReference (i);
        presetBox.addItem (e.name, i + 1);
        if (e.name.equalsIgnoreCase (audioProcessor.getCurrentPresetName()))
            matchId = i + 1;
    }
    presetBox.setSelectedId (matchId, juce::dontSendNotification);
    suppressPresetBoxCallback = false;
}

void CyberverbX1AudioProcessorEditor::handleSavePresetClick (bool forceSaveAsDialog)
{
    if (! forceSaveAsDialog && audioProcessor.isCurrentPresetSaved() && ! audioProcessor.isCurrentPresetFactory())
    {
        if (audioProcessor.savePreset (audioProcessor.getCurrentPresetName(), audioProcessor.getCurrentPresetCategory()))
            triggerLcdNotification ("PRESET SAVED", audioProcessor.getCurrentPresetName());
        return;
    }

    auto* alert = new juce::AlertWindow ("CYBERVERB X1 // SAVE PRESET", "Enter preset name & category:", juce::MessageBoxIconType::NoIcon);
    alert->addTextEditor ("name", "Cyber Space 01", "Preset Name:");
    alert->addTextEditor ("cat", "User", "Category:");
    alert->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    alert->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    alert->enterModalState (true, juce::ModalCallbackFunction::create ([this, alert] (int res)
    {
        if (res == 1)
        {
            const auto n = alert->getTextEditorContents ("name").trim();
            const auto c = alert->getTextEditorContents ("cat").trim();
            if (audioProcessor.savePreset (n, c))
            {
                refreshPresetList();
                triggerLcdNotification ("PRESET SAVED", n);
            }
        }
    }), true);
}

void CyberverbX1AudioProcessorEditor::handleLoadPresetClick()
{
    activeFileChooser = std::make_unique<juce::FileChooser> ("Load Preset", audioProcessor.getPresetDirectory(), "*.xml");
    activeFileChooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                    [this] (const juce::FileChooser& c)
    {
        if (audioProcessor.loadPresetFromFile (c.getResult(), true))
        {
            refreshPresetList();
            triggerLcdNotification ("PRESET LOADED", audioProcessor.getCurrentPresetName());
        }
    });
}

void CyberverbX1AudioProcessorEditor::handleRenameOrMovePresetClick() {}
void CyberverbX1AudioProcessorEditor::handleDeletePresetClick()
{
    if (audioProcessor.deleteCurrentUserPreset())
    {
        refreshPresetList();
        triggerLcdNotification ("PRESET", "DELETED");
    }
}
void CyberverbX1AudioProcessorEditor::handleRestoreFactoryPresetsClick() { audioProcessor.restoreFactoryPresets(); refreshPresetList(); }

void CyberverbX1AudioProcessorEditor::handleFlushClick()
{
    audioProcessor.flushAudioTails();
    smoothedLevelL = smoothedLevelR = peakHoldL = peakHoldR = 0.0f;
    triggerLcdNotification ("BUFFER FLUSH", "ALL TAILS CLEARED");
}

void CyberverbX1AudioProcessorEditor::stepPreset (int delta)
{
    const int total = cachedPresetEntries.size();
    if (total <= 0) return;
    const int next = (presetBox.getSelectedId() - 1 + delta + total) % total;
    presetBox.setSelectedId (next + 1, juce::sendNotificationSync);
}

void CyberverbX1AudioProcessorEditor::timerCallback()
{
    const int learned = audioProcessor.consumeLastLearnedAssignSlot();
    if (learned >= 0)
    {
        updateAssignKnobTooltip (learned);
        triggerLcdNotification ("MIDI LEARNED", "CC " + juce::String (audioProcessor.getAssignedMidiCC (learned)));
        repaint();
    }

    if (showLcdNotify && lcdNotifyTicks > 0 && ! keypadActive)
        if (--lcdNotifyTicks == 0) showLcdNotify = false;

    const float pL = audioProcessor.consumePeakLeft();
    const float pR = audioProcessor.consumePeakRight();
    smoothedLevelL = (pL > smoothedLevelL) ? pL : (smoothedLevelL * 0.84f);
    smoothedLevelR = (pR > smoothedLevelR) ? pR : (smoothedLevelR * 0.84f);

    if (pL >= peakHoldL) { peakHoldL = pL; peakHoldTicksL = 36; }
    else if (peakHoldTicksL > 0) --peakHoldTicksL;
    else peakHoldL *= 0.90f;

    if (pR >= peakHoldR) { peakHoldR = pR; peakHoldTicksR = 36; }
    else if (peakHoldTicksR > 0) --peakHoldTicksR;
    else peakHoldR *= 0.90f;

    if (audioProcessor.consumeMidiActivity()) midiLedHoldTicks = 6;
    else if (midiLedHoldTicks > 0) --midiLedHoldTicks;

    audioProcessor.copyScopeSamples (scopeCache);

    if (! bottomStripBounds.isEmpty())
        repaint (bottomStripBounds.toNearestInt().expanded (2));
    repaint (juce::Rectangle<int> (0, 0, getWidth(), juce::roundToInt (getHeight() * 0.56f)));
}

void CyberverbX1AudioProcessorEditor::triggerLcdNotification (const juce::String& title, const juce::String& value)
{
    lcdNotifyTitle = title; lcdNotifyValue = value;
    showLcdNotify = true; lcdNotifyTicks = 48;
    repaint();
}

void CyberverbX1AudioProcessorEditor::setActiveTab (ActiveTab newTab)
{
    if (currentTab != newTab)
    {
        currentTab = newTab;
        updateTabVisibility();
        repaint();
    }
}

void CyberverbX1AudioProcessorEditor::showSettingsMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader ("CYBERVERB X1 // SYSTEM");
    menu.addItem (1, "Init Patch (Reset)");
    menu.addItem (2, "Open Presets Folder...");
    menu.addSeparator();
    menu.addItem (10, "Soft-Takeover (MIDI Pickup)", true, audioProcessor.isMidiPickupEnabled());
    menu.addItem (11, "Save MIDI Mappings...");
    menu.addItem (12, "Load MIDI Mappings...");
    menu.addItem (13, "Clear All MIDI Mappings");
    menu.addSeparator();
    menu.addItem (20, "UI Scale 80%  (896 x 504)");
    menu.addItem (21, "UI Scale 100% (1120 x 630)");
    menu.addItem (22, "UI Scale 125% (1400 x 788)");
    menu.addItem (23, "UI Scale 150% (1680 x 945)");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&settingsButton), [this] (int r)
    {
        switch (r)
        {
            case 1:  audioProcessor.loadInitPatch(); refreshPresetList(); triggerLcdNotification ("SYSTEM", "INIT LOADED"); break;
            case 2:  audioProcessor.getPresetDirectory().revealToUser(); break;
            case 10: audioProcessor.setMidiPickupEnabled (! audioProcessor.isMidiPickupEnabled()); break;
            case 11: handleSaveMidiMappingClick(); break;
            case 12: handleLoadMidiMappingClick(); break;
            case 13: audioProcessor.clearAllMidiMappings(); break;
            case 20: setSize (896, 504); break;
            case 21: setSize (1120, 630); break;
            case 22: setSize (1400, 788); break;
            case 23: setSize (1680, 945); break;
            default: break;
        }
    });
}

void CyberverbX1AudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (12, 15, 20));
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    if (backgroundImage.isValid())
        g.drawImage (backgroundImage, bounds, juce::RectanglePlacement::stretchToFit);

    const auto& currentTabImg = tabImages[static_cast<size_t> (currentTab)];
    if (currentTabImg.isValid())
    {
        g.drawImage (currentTabImg, bounds, juce::RectanglePlacement::stretchToFit);
    }

    // Linkes Status-LCD
    drawLcdScreen (g, juce::Rectangle<float> (0.0774f * w, 0.2243f * h, (0.2964f - 0.0774f) * w, (0.4053f - 0.2243f) * h));
    drawSystemButtonLeds (g, w, h);
    drawMatrixLeds (g, w, h);
    drawFaderLabels (g, w, h);

    if (! bottomStripBounds.isEmpty())
        drawBottomStatusStrip (g, bottomStripBounds);

    // Center-Display Panels
    const auto leftPanel  = juce::Rectangle<float> (0.3181f * w, 0.2673f * h, 0.2380f * w, 0.2700f * h);
    const auto rightPanel = juce::Rectangle<float> (0.5662f * w, 0.2673f * h, 0.2380f * w, 0.2700f * h);

    switch (currentTab)
    {
        case ActiveTab::Routing:    drawRoutingPanels (g, leftPanel, rightPanel); break;
        case ActiveTab::SpaceA:     drawSpacePanels (g, leftPanel, rightPanel, 0); break;
        case ActiveTab::SpaceB:     drawSpacePanels (g, leftPanel, rightPanel, 1); break;
        case ActiveTab::Modulation: drawModulationPanels (g, leftPanel, rightPanel); break;
        case ActiveTab::EQ:         drawEqPanels (g, leftPanel, rightPanel); break;
        case ActiveTab::Matrix:     drawMatrixPanels (g, leftPanel, rightPanel); break;
        default: break;
    }

    drawAssignMidiLearnBadges (g);
}

void CyberverbX1AudioProcessorEditor::resized()
{
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    bottomStripBounds = juce::Rectangle<float> (0.2661f * w, 0.9016f * h, (0.9113f - 0.2661f) * w, (0.9481f - 0.9016f) * h);

    // Top-Bar
    presetBox.setBounds (juce::roundToInt (0.3600f * w), juce::roundToInt (0.0100f * h),
                         juce::roundToInt (0.2365f * w), juce::roundToInt (0.0390f * h));

    const float topBtnW = 0.054f * w, topBtnH = 0.034f * h;
    savePresetButton.setBounds (juce::roundToInt (0.1905f * w - topBtnW * 0.5f), juce::roundToInt (0.0360f * h - topBtnH * 0.5f), juce::roundToInt (topBtnW), juce::roundToInt (topBtnH));
    loadPresetButton.setBounds (juce::roundToInt (0.2542f * w - topBtnW * 0.5f), juce::roundToInt (0.0360f * h - topBtnH * 0.5f), juce::roundToInt (topBtnW), juce::roundToInt (topBtnH));
    flushButton.setBounds      (juce::roundToInt (0.3190f * w - topBtnW * 0.5f), juce::roundToInt (0.0360f * h - topBtnH * 0.5f), juce::roundToInt (topBtnW), juce::roundToInt (topBtnH));

    const float navBtnW = 0.024f * w, navBtnH = 0.034f * h;
    prevPresetButton.setBounds (juce::roundToInt (0.6173f * w - navBtnW * 0.5f), juce::roundToInt (0.0340f * h - navBtnH * 0.5f), juce::roundToInt (navBtnW), juce::roundToInt (navBtnH));
    nextPresetButton.setBounds (juce::roundToInt (0.6464f * w - navBtnW * 0.5f), juce::roundToInt (0.0340f * h - navBtnH * 0.5f), juce::roundToInt (navBtnW), juce::roundToInt (navBtnH));

    // 6 Tab-Reiter
    const float tabStartX    = 0.305f;
    const float tabY         = 0.208f;
    const float tabH         = 0.036f;
    const float tabWidths[6] = { 0.092f, 0.068f, 0.082f, 0.068f, 0.105f, 0.115f };
    float curX = tabStartX;
    for (size_t i = 0; i < tabButtons.size(); ++i)
    {
        tabButtons[i].setBounds (juce::roundToInt (curX * w), juce::roundToInt (tabY * h),
                                 juce::roundToInt (tabWidths[i] * w), juce::roundToInt (tabH * h));
        curX += tabWidths[i];
    }

    // 8 Matrix-Buttons
    constexpr float matBtnX[4] = { 0.4970f, 0.5369f, 0.5768f, 0.6170f };
    const float matBtnW = 0.032f * w, matBtnH = 0.056f * h;
    for (int col = 0; col < 4; ++col)
    {
        const float cx = matBtnX[col] * w;
        matrixButtons[static_cast<size_t> (col)].setBounds     (juce::roundToInt (cx - matBtnW * 0.5f), juce::roundToInt (0.6720f * h - matBtnH * 0.5f), juce::roundToInt (matBtnW), juce::roundToInt (matBtnH));
        matrixButtons[static_cast<size_t> (col + 4)].setBounds (juce::roundToInt (cx - matBtnW * 0.5f), juce::roundToInt (0.8190f * h - matBtnH * 0.5f), juce::roundToInt (matBtnW), juce::roundToInt (matBtnH));
    }

    // 4 System-Buttons links (BYPASS, MIX LOCK, SYNC, SYSTEM)
    constexpr float sysBtnX[4] = { 0.0988f, 0.1536f, 0.2083f, 0.2637f };
    const float sysBtnW = 0.044f * w, sysBtnH = 0.046f * h;
    for (int i = 0; i < 4; ++i)
        systemButtons[static_cast<size_t> (i)].setBounds (juce::roundToInt (sysBtnX[i] * w - sysBtnW * 0.5f), juce::roundToInt (0.4880f * h - sysBtnH * 0.5f), juce::roundToInt (sysBtnW), juce::roundToInt (sysBtnH));

    // Settings-Icon
    const float iconSize = 0.032f * w;
    settingsButton.setBounds (juce::roundToInt (0.9515f * w - iconSize * 0.5f), juce::roundToInt (0.0343f * h - iconSize * 0.5f), juce::roundToInt (iconSize), juce::roundToInt (iconSize));

    // Numpad
    constexpr float kpColX[3] = { 0.6880f, 0.7355f, 0.7830f };
    constexpr float kpRowY[4] = { 0.6320f, 0.6970f, 0.7620f, 0.8270f };
    const float kpBtnW = 0.0370f * w, kpBtnH = 0.0480f * h;
    for (size_t i = 0; i < 12; ++i)
        keypadButtons[i].setBounds (juce::roundToInt (kpColX[i % 3] * w - kpBtnW * 0.5f), juce::roundToInt (kpRowY[i / 3] * h - kpBtnH * 0.5f), juce::roundToInt (kpBtnW), juce::roundToInt (kpBtnH));

    // 10 rechte Drehregler
    constexpr std::array<std::pair<float, float>, 10> knobCoords = {{
        { 0.8490f, 0.2054f }, { 0.9175f, 0.2087f },
        { 0.8496f, 0.3413f }, { 0.9181f, 0.3424f },
        { 0.8496f, 0.4750f }, { 0.9187f, 0.4772f },
        { 0.8502f, 0.6217f }, { 0.9181f, 0.6261f },
        { 0.8490f, 0.7750f }, { 0.9175f, 0.7728f }
    }};
    const float knobDiameter = 0.056f * w;
    for (size_t i = 0; i < rightKnobs.size(); ++i)
        rightKnobs[i].setBounds (juce::roundToInt (knobCoords[i].first * w - knobDiameter * 0.5f),
                                 juce::roundToInt (knobCoords[i].second * h - knobDiameter * 0.5f),
                                 juce::roundToInt (knobDiameter), juce::roundToInt (knobDiameter));

    // 5 vertikale Fader unten links (exakte X-Positionen der 5 Fader-Schlitze in background_main)
    constexpr float faderX[5] = { 0.1085f, 0.1845f, 0.2823f, 0.3542f, 0.4265f };
    const float faderTopY     = 0.6148f * h - 10.0f;
    const float faderBottomY  = 0.8185f * h + 10.0f;
    const float faderWidth    = 0.036f * w;
    for (size_t i = 0; i < mainFaders.size(); ++i)
        mainFaders[i].setBounds (juce::roundToInt (faderX[i] * w - faderWidth * 0.5f), juce::roundToInt (faderTopY),
                                 juce::roundToInt (faderWidth), juce::roundToInt (faderBottomY - faderTopY));

    // Tab-Inhalte positionieren
    const float leftPx  = 0.3181f * w;
    const float rightPx = 0.5662f * w;
    const float panelY  = 0.2673f * h;
    const float panelW  = 0.2380f * w;
    const float tabKnobSz = 0.035f * w;

    auto place4KnobsRow = [panelW, tabKnobSz] (float px, float cy, juce::Slider& k0, juce::Slider& k1, juce::Slider& k2, juce::Slider& k3)
    {
        juce::Slider* arr[4] = { &k0, &k1, &k2, &k3 };
        for (int c = 0; c < 4; ++c)
        {
            const float cx = px + panelW * (0.14f + static_cast<float> (c) * 0.24f);
            arr[c]->setBounds (juce::roundToInt (cx - tabKnobSz * 0.5f), juce::roundToInt (cy - tabKnobSz * 0.5f),
                               juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));
        }
    };

    // Tab 1: ROUTING
    routingControls.sysModeBox.setBounds (juce::roundToInt (leftPx + 8.0f), juce::roundToInt (panelY + 0.042f * h),
                                          juce::roundToInt (panelW * 0.48f), juce::roundToInt (0.032f * h));
    place4KnobsRow (leftPx,  panelY + 0.195f * h, routingControls.splitFreqKnob, routingControls.driveKnob, routingControls.crossFeedKnob, routingControls.duckAmtKnob);
    place4KnobsRow (rightPx, panelY + 0.195f * h, routingControls.duckAmtKnob, routingControls.duckRelKnob, routingControls.gateThrKnob, routingControls.gateRelKnob);
    // Korrektur: 3 Regler links (Split, Drive, CrossFeed) + 4 Regler rechts (DuckAmt, DuckRel, GateThr, GateRel)
    for (int c = 0; c < 3; ++c)
    {
        juce::Slider* k[3] = { &routingControls.splitFreqKnob, &routingControls.driveKnob, &routingControls.crossFeedKnob };
        const float cx = leftPx + panelW * (0.18f + static_cast<float> (c) * 0.32f);
        k[c]->setBounds (juce::roundToInt (cx - tabKnobSz * 0.5f), juce::roundToInt (panelY + 0.195f * h - tabKnobSz * 0.5f),
                         juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));
    }

    // Tab 2 & 3: SPACE A & SPACE B
    for (size_t s = 0; s < 2; ++s)
    {
        auto& sc = spaceTabControls[s];
        const float boxW = (panelW - 22.0f) * 0.5f;
        sc.dlyModeBox.setBounds (juce::roundToInt (leftPx + 8.0f), juce::roundToInt (panelY + 0.042f * h), juce::roundToInt (boxW), juce::roundToInt (0.032f * h));
        sc.dlySyncBox.setBounds (juce::roundToInt (leftPx + 14.0f + boxW), juce::roundToInt (panelY + 0.042f * h), juce::roundToInt (boxW), juce::roundToInt (0.032f * h));
        sc.revAlgoBox.setBounds (juce::roundToInt (rightPx + 8.0f), juce::roundToInt (panelY + 0.042f * h), juce::roundToInt (panelW - 16.0f), juce::roundToInt (0.032f * h));

        for (int c = 0; c < 3; ++c)
        {
            juce::Slider* lk[3] = { &sc.dlyTimeKnob, &sc.dlyToneKnob, &sc.dlyRevMixKnob };
            juce::Slider* rk[3] = { &sc.preDlyKnob, &sc.diffuseKnob, &sc.modDepthKnob };
            const float lcx = leftPx  + panelW * (0.18f + static_cast<float> (c) * 0.32f);
            const float rcx = rightPx + panelW * (0.18f + static_cast<float> (c) * 0.32f);
            lk[c]->setBounds (juce::roundToInt (lcx - tabKnobSz * 0.5f), juce::roundToInt (panelY + 0.195f * h - tabKnobSz * 0.5f), juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));
            rk[c]->setBounds (juce::roundToInt (rcx - tabKnobSz * 0.5f), juce::roundToInt (panelY + 0.195f * h - tabKnobSz * 0.5f), juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));
        }
    }

    // Tab 4: MODULATION
    {
        const float boxW = (panelW - 22.0f) * 0.5f;
        modDynControls.lfo1ShapeBox.setBounds (juce::roundToInt (leftPx + 8.0f), juce::roundToInt (panelY + 0.042f * h), juce::roundToInt (boxW), juce::roundToInt (0.032f * h));
        modDynControls.lfo2ShapeBox.setBounds (juce::roundToInt (leftPx + 14.0f + boxW), juce::roundToInt (panelY + 0.042f * h), juce::roundToInt (boxW), juce::roundToInt (0.032f * h));

        modDynControls.lfo1RateKnob.setBounds (juce::roundToInt (leftPx + panelW * 0.26f - tabKnobSz * 0.5f), juce::roundToInt (panelY + 0.195f * h - tabKnobSz * 0.5f), juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));
        modDynControls.lfo2RateKnob.setBounds (juce::roundToInt (leftPx + panelW * 0.74f - tabKnobSz * 0.5f), juce::roundToInt (panelY + 0.195f * h - tabKnobSz * 0.5f), juce::roundToInt (tabKnobSz), juce::roundToInt (tabKnobSz));

        place4KnobsRow (rightPx, panelY + 0.195f * h, modDynControls.envSensKnob, modDynControls.envAtkKnob, modDynControls.envRelKnob, modDynControls.envBiasKnob);
    }

    // Tab 5: EQ
    place4KnobsRow (leftPx,  panelY + 0.195f * h, eqControls.lowFreqKnob,  eqControls.lowGainKnob,  eqControls.mid1FreqKnob, eqControls.mid1GainKnob);
    place4KnobsRow (rightPx, panelY + 0.195f * h, eqControls.mid2FreqKnob, eqControls.mid2GainKnob, eqControls.highFreqKnob, eqControls.highGainKnob);

    // Tab 6: MATRIX
    {
        const float rowStartY = panelY + 0.045f * h;
        const float rowStepY  = 0.054f * h;
        const float boxH      = 0.031f * h;
        const float boxW      = (panelW - 20.0f - 0.032f * w) * 0.5f;
        const float amtSz     = 0.025f * w;

        for (size_t m = 0; m < 4; ++m)
        {
            auto& slot = modMatrixSlots[m];
            const float ry = rowStartY + static_cast<float> (m) * rowStepY;
            slot.srcBox.setBounds  (juce::roundToInt (leftPx + 8.0f), juce::roundToInt (ry), juce::roundToInt (boxW), juce::roundToInt (boxH));
            slot.dstBox.setBounds  (juce::roundToInt (leftPx + 12.0f + boxW), juce::roundToInt (ry), juce::roundToInt (boxW), juce::roundToInt (boxH));
            slot.amtKnob.setBounds (juce::roundToInt (leftPx + panelW - 8.0f - amtSz), juce::roundToInt (ry + (boxH - amtSz) * 0.5f), juce::roundToInt (amtSz), juce::roundToInt (amtSz));
        }
    }
}

void CyberverbX1AudioProcessorEditor::mouseDown (const juce::MouseEvent& event)
{
    if (event.mods.isLeftButtonDown() && scopeClickArea.contains (event.position))
    {
        bottomStripMode = static_cast<BottomStripMode> ((static_cast<int> (bottomStripMode) + 1) % 2);
        repaint (bottomStripBounds.toNearestInt().expanded (2));
        return;
    }

    if (event.mods.isRightButtonDown())
    {
        const float relX = event.position.x / static_cast<float> (getWidth());
        const float relY = event.position.y / static_cast<float> (getHeight());
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
            "CyberverbX1 Koordinaten-Helfer",
            juce::String::formatted ("Pixel: %d, %d\nRelativ: %.4ff, %.4ff", event.x, event.y, relX, relY));
    }
}