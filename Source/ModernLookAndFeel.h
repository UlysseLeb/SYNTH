/*
  ==============================================================================

    ModernLookAndFeel.h - VINTAGE EDITION

    📌 RÔLE : Style graphique vintage inspiré des synthés analogiques

    🎨 STYLE :
    - Knobs style années 70-80 (Moog, ARP, Roland)
    - Textures brossées métalliques
    - Couleurs chaudes vintage
    - Design rétro authentique

    💡 INSPIRATION :
    - Moog Minimoog (orange & bois)
    - ARP Odyssey (noir & orange)
    - Roland Jupiter-8 (gris métallique)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class ModernLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ModernLookAndFeel()
    {
        // 🎨 Palette de couleurs VINTAGE
        // Orange chaud inspiré des Moog
        vintageCream = juce::Colour(0xfff4e6d8);
        vintageOrange = juce::Colour(0xffff8c42);
        vintageBrown = juce::Colour(0xff8b4513);
        vintageDarkBrown = juce::Colour(0xff3d2817);
        vintageGold = juce::Colour(0xffd4af37);
        vintageMetal = juce::Colour(0xff8a8a8a);

        setColour(juce::Slider::thumbColourId, vintageOrange);
        setColour(juce::Slider::rotarySliderFillColourId, vintageOrange);
        setColour(juce::Slider::rotarySliderOutlineColourId, vintageDarkBrown);
        setColour(juce::Slider::textBoxTextColourId, vintageCream);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0x00000000));

        setColour(juce::ComboBox::backgroundColourId, vintageDarkBrown);
        setColour(juce::ComboBox::textColourId, vintageOrange);
        setColour(juce::ComboBox::outlineColourId, vintageOrange);
        setColour(juce::ComboBox::arrowColourId, vintageOrange);
    }

private:
    juce::Colour vintageCream;
    juce::Colour vintageOrange;
    juce::Colour vintageBrown;
    juce::Colour vintageDarkBrown;
    juce::Colour vintageGold;
    juce::Colour vintageMetal;

public:

    // 🎛️ KNOB MODERNE STYLE (flat design épuré)
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        // Vérification de sécurité pour éviter les dimensions invalides
        if (width <= 0 || height <= 0)
            return;

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(5);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

        // Vérification de sécurité : radius minimum
        if (radius < 10.0f)
            return;

        auto centerX = bounds.getCentreX();
        auto centerY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // 🔴 Track de fond (arc complet gris)
        auto arcRadius = juce::jmax(5.0f, radius - 8.0f);  // Sécurité : minimum 5px
        auto arcThickness = 6.0f;

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                                   0.0f, rotaryStartAngle, rotaryEndAngle, true);

        g.setColour(juce::Colour(0xff3a3a3a));  // Gris foncé
        g.strokePath(backgroundArc, juce::PathStrokeType(arcThickness,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 🟠 Arc de valeur (partie active orange)
        juce::Path valueArc;
        valueArc.addCentredArc(centerX, centerY, arcRadius, arcRadius,
                               0.0f, rotaryStartAngle, angle, true);

        g.setColour(vintageOrange);  // Orange vif
        g.strokePath(valueArc, juce::PathStrokeType(arcThickness,
                    juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // 🔘 Cercle central (knob body)
        auto knobRadius = juce::jmax(3.0f, radius - 16.0f);  // Sécurité : minimum 3px

        // Fond sombre avec léger dégradé
        juce::ColourGradient knobGradient(
            juce::Colour(0xff2a2a2a), centerX, centerY - knobRadius,
            juce::Colour(0xff1a1a1a), centerX, centerY + knobRadius,
            false);
        g.setGradientFill(knobGradient);
        g.fillEllipse(centerX - knobRadius, centerY - knobRadius, knobRadius * 2, knobRadius * 2);

        // 📍 Indicateur de position (trait simple et clair)
        auto indicatorLength = knobRadius * 0.6f;
        auto indicatorThickness = 2.5f;

        // Calculer les coordonnées de l'indicateur avec rotation
        auto indicatorStartX = centerX + (knobRadius * 0.3f) * std::sin(angle);
        auto indicatorStartY = centerY - (knobRadius * 0.3f) * std::cos(angle);
        auto indicatorEndX = centerX + (knobRadius * 0.9f) * std::sin(angle);
        auto indicatorEndY = centerY - (knobRadius * 0.9f) * std::cos(angle);

        g.setColour(vintageOrange.brighter(0.3f));
        g.drawLine(indicatorStartX, indicatorStartY, indicatorEndX, indicatorEndY, indicatorThickness);

        // 🔘 Bordure subtile du knob
        g.setColour(juce::Colour(0xff404040));
        g.drawEllipse(centerX - knobRadius, centerY - knobRadius, knobRadius * 2, knobRadius * 2, 1.5f);
    }

    // 📝 DESSIN DU TEXTBOX DU SLIDER
    // 📝 Explication : Affiche la valeur sous le knob
    //    - Style minimaliste, texte centré
    //    - Pas de bordure visible (moderne)
    juce::Label* createSliderTextBox(juce::Slider& slider) override
    {
        auto* label = LookAndFeel_V4::createSliderTextBox(slider);
        label->setJustificationType(juce::Justification::centred);
        label->setFont(juce::Font(14.0f, juce::Font::bold));
        label->setColour(juce::Label::textColourId, juce::Colours::white);
        label->setColour(juce::Label::backgroundColourId, juce::Colour(0x00000000));
        label->setColour(juce::Label::outlineColourId, juce::Colour(0x00000000));
        return label;
    }

    // 🎨 COMBOBOX VINTAGE (style bouton mécanique)
    void drawComboBox(juce::Graphics& g, int width, int height,
                     bool isButtonDown, int buttonX, int buttonY,
                     int buttonW, int buttonH, juce::ComboBox& box) override
    {
        auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();

        // Fond bois foncé vintage
        juce::ColourGradient gradient(
            vintageDarkBrown.brighter(0.15f), 0, 0,
            vintageDarkBrown.darker(0.1f), 0, (float)height,
            false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(bounds, 3.0f);

        // Bordure orange vintage (comme les Moog)
        g.setColour(vintageOrange);
        g.drawRoundedRectangle(bounds.reduced(1), 3.0f, 2.5f);

        // Bordure intérieure dorée
        g.setColour(vintageGold.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.reduced(3), 2.0f, 1.0f);

        // Flèche vintage (plus grosse et stylisée)
        auto arrowZone = juce::Rectangle<int>(buttonX, buttonY, buttonW, buttonH).toFloat();
        juce::Path arrow;
        arrow.addTriangle(arrowZone.getX() + 4.0f, arrowZone.getCentreY() - 3.0f,
                         arrowZone.getRight() - 4.0f, arrowZone.getCentreY() - 3.0f,
                         arrowZone.getCentreX(), arrowZone.getCentreY() + 4.0f);

        g.setColour(vintageOrange.withAlpha(box.isEnabled() ? 1.0f : 0.4f));
        g.fillPath(arrow);
    }
};
