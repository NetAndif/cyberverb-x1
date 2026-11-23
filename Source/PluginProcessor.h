#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

class CyberverbX1AudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int scopeBufferSize = 256;
    static constexpr int numAssignKnobs  = 8; // Rechte 8 Engine-Regler (Decay/Size/Damp/DlyFeed A & B)

    CyberverbX1AudioProcessor();
    ~CyberverbX1AudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Engine Copy / Swap / Reset / Randomize
    void copySpaceParameters (int sourceSpace, int targetSpace);
    void swapSpaceParameters();
    void resetSpaceParameters (int space);
    void randomizeSpaceParameters (int space);

    // MIDI Learn & Soft-Takeover
    enum class MidiCCMode : int
    {
        Absolute = 0,
        RelativeTwosComp = 1,
        RelativeBinOffset = 2
    };

    void startMidiLearnForAssign (int assignIndex) noexcept
    {
        if (assignIndex >= 0 && assignIndex < numAssignKnobs)
            midiLearnTargetSlot.store (assignIndex, std::memory_order_release);
    }

    void cancelMidiLearn() noexcept { midiLearnTargetSlot.store (-1, std::memory_order_release); }
    int getMidiLearnTargetSlot() const noexcept { return midiLearnTargetSlot.load (std::memory_order_acquire); }

    int getAssignedMidiCC (int assignIndex) const noexcept
    {
        if (assignIndex >= 0 && assignIndex < numAssignKnobs)
            return assignKnobMidiCC[static_cast<size_t> (assignIndex)].load (std::memory_order_acquire);
        return -1;
    }

    int consumeLastLearnedAssignSlot() noexcept { return lastLearnedAssignSlot.exchange (-1, std::memory_order_acq_rel); }

    void setMidiPickupEnabled (bool enabled) noexcept
    {
        midiPickupEnabled.store (enabled, std::memory_order_release);
        if (enabled) resetAllPickupLocks();
    }

    bool isMidiPickupEnabled() const noexcept { return midiPickupEnabled.load (std::memory_order_acquire); }

    bool isAssignWaitingForPickup (int assignIndex) const noexcept
    {
        if (! isMidiPickupEnabled() || assignIndex < 0 || assignIndex >= numAssignKnobs) return false;
        if (getAssignMidiCCMode (assignIndex) != MidiCCMode::Absolute) return false;
        if (assignKnobMidiCC[static_cast<size_t> (assignIndex)].load (std::memory_order_acquire) < 0) return false;
        return ! assignPickupLocked[static_cast<size_t> (assignIndex)].load (std::memory_order_acquire);
    }

    void resetAllPickupLocks() noexcept
    {
        for (int k = 0; k < numAssignKnobs; ++k)
        {
            assignPickupLocked[static_cast<size_t> (k)].store (false, std::memory_order_release);
            lastMidiCCNorm[static_cast<size_t> (k)].store (-1.0f, std::memory_order_release);
        }
    }

    void clearMidiLearnForAssign (int assignIndex) noexcept
    {
        if (assignIndex >= 0 && assignIndex < numAssignKnobs)
        {
            const auto idx = static_cast<size_t> (assignIndex);
            assignKnobMidiCC[idx].store (-1, std::memory_order_release);
            assignPickupLocked[idx].store (false, std::memory_order_release);
            lastMidiCCNorm[idx].store (-1.0f, std::memory_order_release);
            assignKnobCCMode[idx].store (static_cast<int> (MidiCCMode::Absolute), std::memory_order_relaxed);
            if (midiLearnTargetSlot.load (std::memory_order_acquire) == assignIndex)
                midiLearnTargetSlot.store (-1, std::memory_order_release);
        }
    }

    void clearAllMidiMappings() noexcept
    {
        midiLearnTargetSlot.store (-1, std::memory_order_release);
        for (int k = 0; k < numAssignKnobs; ++k)
            clearMidiLearnForAssign (k);
    }

    void setAssignMidiCCMode (int assignIndex, MidiCCMode mode) noexcept
    {
        if (assignIndex >= 0 && assignIndex < numAssignKnobs)
        {
            const auto idx = static_cast<size_t> (assignIndex);
            assignKnobCCMode[idx].store (static_cast<int> (mode), std::memory_order_release);
            assignPickupLocked[idx].store (false, std::memory_order_release);
            lastMidiCCNorm[idx].store (-1.0f, std::memory_order_release);
        }
    }

    MidiCCMode getAssignMidiCCMode (int assignIndex) const noexcept
    {
        if (assignIndex >= 0 && assignIndex < numAssignKnobs)
            return static_cast<MidiCCMode> (assignKnobCCMode[static_cast<size_t> (assignIndex)].load (std::memory_order_acquire));
        return MidiCCMode::Absolute;
    }

    juce::File getMidiMapDirectory() const;
    bool saveMidiMappingToFile (const juce::File& file) const;
    bool loadMidiMappingFromFile (const juce::File& file);

    // Preset-Verwaltung
    struct PresetEntry
    {
        juce::String name;
        juce::String category;
        juce::String author;
        juce::String notes;
        juce::File file;
        bool isFactory = false;
        bool isInit    = false;
    };

    juce::File getPresetDirectory() const;
    juce::File getFactoryPresetDirectory() const { return getPresetDirectory().getChildFile ("Factory"); }
    juce::File getUserPresetDirectory() const    { return getPresetDirectory().getChildFile ("User"); }

    void ensureDefaultPresetExists();
    void installFactoryPresets (bool overwriteExisting = false);

    juce::Array<PresetEntry> getAllPresetEntries() const;
    juce::StringArray getAvailableCategories() const;
    juce::StringArray getPresetCategories() const { return getAvailableCategories(); }

    bool savePreset (const juce::String& presetName,
                     const juce::String& category = "User",
                     const juce::String& author = {},
                     const juce::String& notes = {});
    bool loadPresetByName (const juce::String& presetName);
    bool loadPresetEntry (const PresetEntry& entry);
    bool loadPresetFromFile (const juce::File& file, bool importToLibrary, const juce::String& targetCategory = "User");
    bool renameOrMoveCurrentUserPreset (const juce::String& newName, const juce::String& newCategory);
    bool deleteCurrentUserPreset();
    void restoreFactoryPresets();

    void loadInitPatch();
    void flushAudioTails();

    juce::String getCurrentPresetName() const     { return currentPresetName; }
    juce::String getCurrentPresetCategory() const { return currentPresetCategory; }
    juce::String getCurrentPresetAuthor() const   { return currentPresetAuthor; }
    juce::String getCurrentPresetNotes() const    { return currentPresetNotes; }
    bool isCurrentPresetSaved() const noexcept    { return currentPresetIsSaved; }
    bool isCurrentPresetFactory() const noexcept  { return currentPresetIsFactory; }

    // Globale System-Flags (UI-gesteuert & gespeichert)
    void setMixLockEnabled (bool b) noexcept { mixLockEnabled.store (b, std::memory_order_release); }
    bool isMixLockEnabled() const noexcept   { return mixLockEnabled.load (std::memory_order_acquire); }

    void setGlobalSyncEnabled (bool b) noexcept { globalSyncEnabled.store (b, std::memory_order_release); }
    bool isGlobalSyncEnabled() const noexcept   { return globalSyncEnabled.load (std::memory_order_acquire); }

    // Telemetrie für UI
    float consumeInputPeak() noexcept   { return inputPeakLevel.exchange (0.0f); }
    float consumePeakLeft() noexcept    { return outputPeakL.exchange (0.0f); }
    float consumePeakRight() noexcept   { return outputPeakR.exchange (0.0f); }
    float getLiveEnvFollower() const noexcept { return liveEnvFollowerOut.load (std::memory_order_relaxed); }
    float getLiveLfo1Value() const noexcept   { return liveLfo1Out.load (std::memory_order_relaxed); }
    float getLiveLfo2Value() const noexcept   { return liveLfo2Out.load (std::memory_order_relaxed); }
    float getLiveEffectiveMorph() const noexcept { return liveEffectiveMorph.load (std::memory_order_relaxed); }
    float getHostBpm() const noexcept            { return currentBpm.load (std::memory_order_relaxed); }
    double getCurrentSampleRateHz() const noexcept { return currentSampleRateHz.load (std::memory_order_relaxed); }
    bool consumeMidiActivity() noexcept          { return midiActivityFlag.exchange (false); }

    void copyScopeSamples (std::array<float, scopeBufferSize>& dest) const noexcept
    {
        const int writeIdx = scopeWritePos.load (std::memory_order_acquire);
        for (int i = 0; i < scopeBufferSize; ++i)
        {
            const int srcIdx = (writeIdx + i) % scopeBufferSize;
            dest[static_cast<size_t> (i)] = scopeRingBuffer[static_cast<size_t> (srcIdx)].load (std::memory_order_relaxed);
        }
    }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void cacheParameterPointers();
    void updateWetEqFilters();

    struct SpaceParamPointers
    {
        std::atomic<float>* on        = nullptr;
        std::atomic<float>* dlyTime   = nullptr;
        std::atomic<float>* dlySync   = nullptr;
        std::atomic<float>* dlyFeed   = nullptr;
        std::atomic<float>* dlyMode   = nullptr;
        std::atomic<float>* dlyTone   = nullptr;
        std::atomic<float>* dlyRevMix = nullptr;

        std::atomic<float>* revAlgo    = nullptr;
        std::atomic<float>* revDecay   = nullptr;
        std::atomic<float>* revSize    = nullptr;
        std::atomic<float>* revDamp    = nullptr;
        std::atomic<float>* revPreDly  = nullptr;
        std::atomic<float>* revDiffuse = nullptr;
        std::atomic<float>* revMod     = nullptr;
    };

    struct ModSlotPointers
    {
        std::atomic<float>* src    = nullptr;
        std::atomic<float>* dst    = nullptr;
        std::atomic<float>* amount = nullptr;
    };

    struct ParamCache
    {
        std::array<SpaceParamPointers, 2> spaces;

        // Global / Mix / Master
        std::atomic<float>* bypass      = nullptr;
        std::atomic<float>* freeze      = nullptr;
        std::atomic<float>* mixDry      = nullptr;
        std::atomic<float>* mixWet      = nullptr;
        std::atomic<float>* spaceALevel = nullptr;
        std::atomic<float>* spaceMorph  = nullptr;
        std::atomic<float>* spaceBLevel = nullptr;
        std::atomic<float>* outGainDb   = nullptr;
        std::atomic<float>* outWidth    = nullptr;

        // Routing & Dynamics
        std::atomic<float>* sysMode      = nullptr;
        std::atomic<float>* sysSplitFreq = nullptr;
        std::atomic<float>* sysDrive     = nullptr;
        std::atomic<float>* sysCrossFeed = nullptr;
        std::atomic<float>* duckAmt      = nullptr;
        std::atomic<float>* duckRel      = nullptr;
        std::atomic<float>* gateThr      = nullptr;
        std::atomic<float>* gateRel      = nullptr;

        // Modulation
        std::atomic<float>* lfo1Rate  = nullptr;
        std::atomic<float>* lfo1Shape = nullptr;
        std::atomic<float>* lfo2Rate  = nullptr;
        std::atomic<float>* lfo2Shape = nullptr;
        std::atomic<float>* envSens   = nullptr;
        std::atomic<float>* envAtk    = nullptr;
        std::atomic<float>* envRel    = nullptr;
        std::atomic<float>* envBias   = nullptr;

        // Wet EQ
        std::atomic<float>* eqLowFreq   = nullptr;
        std::atomic<float>* eqLowGain   = nullptr;
        std::atomic<float>* eqMid1Freq  = nullptr;
        std::atomic<float>* eqMid1Gain  = nullptr;
        std::atomic<float>* eqMid2Freq  = nullptr;
        std::atomic<float>* eqMid2Gain  = nullptr;
        std::atomic<float>* eqHighFreq  = nullptr;
        std::atomic<float>* eqHighGain  = nullptr;

        // Matrix
        std::array<ModSlotPointers, 4> modSlots;
    } params;

    // Interne DSP-Instanzen pro Space (Delay + Reverb + Filter)
    struct SpaceDspUnit
    {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> dlyLineL { 192000 };
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> dlyLineR { 192000 };
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>      preDlyL  { 96000 };
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>      preDlyR  { 96000 };

        juce::dsp::StateVariableTPTFilter<float> dlyToneFilterL;
        juce::dsp::StateVariableTPTFilter<float> dlyToneFilterR;
        juce::dsp::Reverb reverb;

        float fbStateL = 0.0f;
        float fbStateR = 0.0f;
        float modPhase = 0.0f;

        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
    };

    std::array<SpaceDspUnit, 2> dspSpaces;

    // Crossover & Wet EQ
    juce::dsp::LinkwitzRileyFilter<float> crossoverLow;
    juce::dsp::LinkwitzRileyFilter<float> crossoverHigh;

    using IIRFilter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>;
    IIRFilter wetEqLow, wetEqMid1, wetEqMid2, wetEqHigh;

    // Modulations- & Dynamik-Zustände
    float lfo1Phase = 0.0f;
    float lfo2Phase = 0.0f;
    float lfo1SmoothedSH = 0.0f;
    float lfo2SmoothedSH = 0.0f;
    float envFollowerState = 0.0f;
    float duckGainState    = 1.0f;
    float gateGainState    = 1.0f;
    float crossFbAtoB      = 0.0f;
    float crossFbBtoA      = 0.0f;

    juce::String currentPresetName     { "Init" };
    juce::String currentPresetCategory {};
    juce::String currentPresetAuthor   {};
    juce::String currentPresetNotes    {};
    juce::File   currentPresetFile     {};
    bool currentPresetIsSaved   = false;
    bool currentPresetIsFactory = false;

    std::atomic<bool> flushRequested    { false };
    std::atomic<bool> mixLockEnabled    { false };
    std::atomic<bool> globalSyncEnabled { false };

    std::atomic<double> currentSampleRateHz { 44100.0 };
    std::atomic<float>  currentBpm          { 120.0f };
    std::atomic<float>  inputPeakLevel      { 0.0f };
    std::atomic<float>  outputPeakL         { 0.0f };
    std::atomic<float>  outputPeakR         { 0.0f };
    std::atomic<float>  liveEnvFollowerOut  { 0.0f };
    std::atomic<float>  liveLfo1Out         { 0.0f };
    std::atomic<float>  liveLfo2Out         { 0.0f };
    std::atomic<float>  liveEffectiveMorph  { 0.5f };
    std::atomic<bool>   midiActivityFlag    { false };

    std::array<std::atomic<float>, scopeBufferSize> scopeRingBuffer {};
    std::atomic<int> scopeWritePos { 0 };
    int scopeDecimator = 0;

    std::array<std::atomic<int>, numAssignKnobs>   assignKnobMidiCC {};
    std::atomic<int>                               midiLearnTargetSlot   { -1 };
    std::atomic<int>                               lastLearnedAssignSlot { -1 };
    std::atomic<bool>                              midiPickupEnabled     { true };
    std::array<std::atomic<bool>, numAssignKnobs>  assignPickupLocked {};
    std::array<std::atomic<float>, numAssignKnobs> lastMidiCCNorm {};
    std::array<std::atomic<int>, numAssignKnobs>   assignKnobCCMode {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CyberverbX1AudioProcessor)
};