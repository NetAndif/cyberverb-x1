#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace CyberverbColours
{
    inline const juce::Colour engineA       = juce::Colour::fromRGB (255, 106,   0); // Warmes Cyber-Orange (#FF6A00)
    inline const juce::Colour engineB       = juce::Colour::fromRGB (255, 208,   0); // Gold-Gelb (#FFD000)
    inline const juce::Colour textPrimary   = juce::Colour (0xffd4d8de);
    inline const juce::Colour panelDark     = juce::Colour::fromRGB (14,  18,  24);
    inline const juce::Colour borderMetal   = juce::Colour::fromRGB (58,  68,  82);
}

class CyberverbLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CyberverbLookAndFeel()
    {
        customTypeface = juce::Typeface::createSystemTypefaceFor (
            BinaryData::RajdhaniSemiBold_ttf,
            BinaryData::RajdhaniSemiBold_ttfSize
        );

        setDefaultSansSerifTypeface (customTypeface);

        setColour (juce::Label::textColourId, CyberverbColours::textPrimary);
        setColour (juce::ComboBox::textColourId, CyberverbColours::textPrimary);
        setColour (juce::PopupMenu::textColourId, CyberverbColours::textPrimary);
        setColour (juce::PopupMenu::backgroundColourId, CyberverbColours::panelDark);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, CyberverbColours::engineA.withAlpha (0.24f));
        setColour (juce::PopupMenu::highlightedTextColourId, CyberverbColours::engineB);
    }

    juce::Typeface::Ptr getTypefaceForFont (const juce::Font& /*font*/) override
    {
        return customTypeface;
    }

    // 1. Matrix- & Parameter-Drehregler (unipolar & bipolar)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                    static_cast<float> (width), static_cast<float> (height)).reduced (3.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto centre = bounds.getCentre();
        const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
        const bool isBipolar = static_cast<bool> (slider.getProperties().getWithDefault ("isBipolar", false));

        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
        const float arcRadius = radius - 2.0f;

        // Äußerer Hintergrund-Track
        juce::Path bgArc;
        bgArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour::fromRGB (24, 30, 38).withAlpha (0.88f));
        g.strokePath (bgArc, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Aktiver Wert-Arc (ab Mitte bei bipolar, ab Start bei unipolar)
        const float zeroAngle = isBipolar
                                    ? (rotaryStartAngle + rotaryEndAngle) * 0.5f
                                    : rotaryStartAngle;

        if (std::abs (angle - zeroAngle) > 0.01f)
        {
            juce::Path valArc;
            valArc.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f,
                                  std::min (zeroAngle, angle), std::max (zeroAngle, angle), true);

            g.setColour (accent.withAlpha (0.28f));
            g.strokePath (valArc, juce::PathStrokeType (4.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour (accent);
            g.strokePath (valArc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Metallische Kappe innen
        const float capRadius = radius * 0.72f;
        juce::ColourGradient capGrad (juce::Colour::fromRGB (34, 40, 48), centre.x, centre.y - capRadius,
                                      juce::Colour::fromRGB (12, 16, 22), centre.x, centre.y + capRadius, false);
        g.setGradientFill (capGrad);
        g.fillEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f);

        g.setColour (CyberverbColours::borderMetal.withAlpha (0.75f));
        g.drawEllipse (centre.x - capRadius, centre.y - capRadius, capRadius * 2.0f, capRadius * 2.0f, 1.0f);

        // Leuchtender Zeigerstrich
        juce::Path pointer;
        const float ptrLength = capRadius * 0.85f;
        const float ptrThickness = 2.2f;
        pointer.addRoundedRectangle (-ptrThickness * 0.5f, -ptrLength, ptrThickness, ptrLength * 0.58f, 1.0f);
        pointer.applyTransform (juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));

        g.setColour (accent);
        g.fillPath (pointer);
    }

    // 2. Vertikale Fader (DRY, WET, SPACE A, MORPH, SPACE B)
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float /*minSliderPos*/, float /*maxSliderPos*/,
                           const juce::Slider::SliderStyle /*style*/, juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                    static_cast<float> (width), static_cast<float> (height));
        const auto accent = slider.findColour (juce::Slider::thumbColourId);

        // Fader Track
        const float trackW = 4.0f;
        auto trackRect = juce::Rectangle<float> (bounds.getCentreX() - trackW * 0.5f,
                                                 bounds.getY() + 8.0f,
                                                 trackW,
                                                 bounds.getHeight() - 16.0f);

        g.setColour (juce::Colour::fromRGB (12, 15, 20).withAlpha (0.92f));
        g.fillRoundedRectangle (trackRect, 2.0f);

        // Aktiver Bereich unterhalb des Fader-Knopfes
        const float clampedPos = juce::jlimit (trackRect.getY(), trackRect.getBottom(), sliderPos);
        auto activeTrack = trackRect.withTop (clampedPos);
        g.setColour (accent.withAlpha (0.80f));
        g.fillRoundedRectangle (activeTrack, 2.0f);

        // Fader-Kappe
        const float capW = bounds.getWidth() * 0.78f;
        const float capH = 16.0f;
        auto capRect = juce::Rectangle<float> (bounds.getCentreX() - capW * 0.5f,
                                               clampedPos - capH * 0.5f,
                                               capW,
                                               capH);

        g.setColour (juce::Colour::fromRGB (22, 27, 35));
        g.fillRoundedRectangle (capRect, 3.0f);
        g.setColour (accent);
        g.drawRoundedRectangle (capRect, 3.0f, 1.2f);

        // Leuchtende Mittellinie auf der Fader-Kappe
        g.drawHorizontalLine (juce::roundToInt (capRect.getCentreY()),
                              capRect.getX() + 3.0f,
                              capRect.getRight() - 3.0f);
    }

    // 3. Dropdown (ComboBox)
    void drawComboBox (juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                       int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                       juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<float> (0.0f, 0.0f,
                                                    static_cast<float> (width),
                                                    static_cast<float> (height)).reduced (0.5f);

        g.setColour (CyberverbColours::panelDark.withAlpha (0.92f));
        g.fillRoundedRectangle (bounds, 4.0f);

        g.setColour (box.hasKeyboardFocus (true)
                         ? CyberverbColours::engineA
                         : CyberverbColours::borderMetal.withAlpha (0.85f));
        g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

        // Kleiner Pfeil rechts
        const float arrowX = static_cast<float> (width) - 13.0f;
        const float arrowY = static_cast<float> (height) * 0.5f;
        juce::Path arrow;
        arrow.addTriangle (arrowX - 3.5f, arrowY - 2.0f,
                           arrowX + 3.5f, arrowY - 2.0f,
                           arrowX,        arrowY + 2.5f);

        g.setColour (CyberverbColours::textPrimary);
        g.fillPath (arrow);
    }

    juce::Font getComboBoxFont (juce::ComboBox& box) override
    {
        return juce::Font (juce::FontOptions (juce::jlimit (10.5f, 13.5f, static_cast<float> (box.getHeight()) * 0.58f)));
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (6, 1, box.getWidth() - 20, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setColour (juce::Label::textColourId, CyberverbColours::textPrimary);
    }

    // 4. Popup-Menüs
    juce::Font getPopupMenuFont() override
    {
        return juce::Font (juce::FontOptions (13.5f));
    }

    // 5. Buttons
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& /*backgroundColour*/,
                               bool shouldDrawButtonAsHighlighted,
                               bool /*shouldDrawButtonAsDown*/) override
    {
        const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
        const bool isOn   = button.getToggleState();
        const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

        if (isOn)
        {
            g.setColour (accent.withAlpha (0.22f));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (accent);
            g.drawRoundedRectangle (bounds, 4.0f, 1.4f);
        }
        else
        {
            g.setColour (juce::Colour::fromRGB (18, 22, 30).withAlpha (shouldDrawButtonAsHighlighted ? 0.95f : 0.80f));
            g.fillRoundedRectangle (bounds, 4.0f);
            g.setColour (CyberverbColours::borderMetal.withAlpha (shouldDrawButtonAsHighlighted ? 0.9f : 0.6f));
            g.drawRoundedRectangle (bounds, 4.0f, 1.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/) override
    {
        const bool isOn   = button.getToggleState();
        const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

        g.setColour (isOn ? accent : CyberverbColours::textPrimary);
        g.setFont (juce::FontOptions (12.0f));
        g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
    }

private:
    juce::Typeface::Ptr customTypeface;
};