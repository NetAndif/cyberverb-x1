#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class TopBarPresetButton : public juce::TextButton
{
public:
    std::function<void()> onRightClick;

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() && onRightClick != nullptr)
        {
            onRightClick();
            return;
        }
        juce::TextButton::mouseUp (e);
    }
};

class AssignableKnobSlider : public juce::Slider
{
public:
    std::function<void()> onRightClick;

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown() && onRightClick != nullptr)
        {
            onRightClick();
            return;
        }
        juce::Slider::mouseDown (e);
    }
};