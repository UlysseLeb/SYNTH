/*
  ==============================================================================

    PluginEditor.cpp - VERSION MODERNE PROFESSIONNELLE

    📌 NOUVELLE INTERFACE :
    - Knobs rotatifs (au lieu de sliders)
    - Analyseur de spectre temps réel
    - Style moderne (dégradés, ombres)
    - Layout optimisé

  ==============================================================================
*/

#include "PluginEditor.h"

// ================= Constructeur =================
SYNTH_1AudioProcessorEditor::SYNTH_1AudioProcessorEditor(SYNTH_1AudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      keyboardComponent(p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // 🎨 ÉTAPE 1 : Appliquer le style custom à TOUS les composants
    // 📝 Explication : setLookAndFeel applique notre design moderne
    //    - Tous les sliders deviennent des knobs stylisés
    //    - ComboBox prend le style moderne
    //    - Labels gardent leurs propriétés custom
    setLookAndFeel(&modernLookAndFeel);

    // ================= Configuration du clavier MIDI =================
    addAndMakeVisible(keyboardComponent);

    // ================= Configuration de l'analyseur de spectre =================
    addAndMakeVisible(spectrumAnalyzer);

    // ================= Helper lambda pour configurer un KNOB =================
    // 📝 Explication : Fonction locale pour éviter la répétition
    //    - Configure tous les knobs avec le même style
    //    - Rotary = knob rotatif (au lieu de LinearVertical)
    auto setupKnob = [this](juce::Slider& knob)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        addAndMakeVisible(knob);
    };

    // ================= Helper lambda pour configurer un SLIDER VERTICAL (ADSR) =================
    // 📝 Explication : Fonction pour les sliders verticaux (enveloppes ADSR)
    //    - LinearVertical = slider vertical (comme sur les synthés hardware)
    //    - Parfait pour les enveloppes ADSR (visualisation intuitive)
    //    - Sensibilité réduite pour un contrôle plus précis
    auto setupVerticalSlider = [this](juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::LinearVertical);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
        slider.setVelocityBasedMode(true);           // Mode basé sur la vélocité
        slider.setVelocityModeParameters(0.3, 1, 0.09, false);  // Sensibilité réduite (0.3 au lieu de 1.0)
        addAndMakeVisible(slider);
    };

    // ================= Configuration des sliders ADSR (VERTICAUX!) =================
    setupVerticalSlider(attackKnob);
    setupVerticalSlider(decayKnob);
    setupVerticalSlider(sustainKnob);
    setupVerticalSlider(releaseKnob);

    // ================= Configuration des knobs Filtre =================
    setupKnob(cutoffKnob);
    setupKnob(resonanceKnob);
    setupKnob(filterEnvAmountKnob);

    // ================= Configuration des sliders ADSR Filtre (VERTICAUX!) =================
    setupVerticalSlider(filterAttackKnob);
    setupVerticalSlider(filterDecayKnob);
    setupVerticalSlider(filterSustainKnob);
    setupVerticalSlider(filterReleaseKnob);

    // ================= Configuration des knobs Unison =================
    setupKnob(voicesKnob);
    setupKnob(detuneKnob);
    setupKnob(stereoKnob);

    // ================= Configuration du sélecteur de forme d'onde =================
    waveformSelector.addItem("Sine", 1);
    waveformSelector.addItem("Saw", 2);
    waveformSelector.addItem("Square", 3);
    waveformSelector.addItem("Triangle", 4);
    waveformSelector.setSelectedId(1);
    addAndMakeVisible(waveformSelector);

    // ================= Configuration des contrôles NOISE (NOUVEAU!) =================
    // 🔊 Toggle button pour activer/désactiver le bruit
    noiseEnableButton.setButtonText("NOISE");
    noiseEnableButton.setClickingTogglesState(true);
    addAndMakeVisible(noiseEnableButton);

    // 🎚️ Knob pour le niveau du bruit
    setupKnob(noiseLevelKnob);

    // ================= Configuration des labels VINTAGE =================
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colour(0xfff4e6d8)); // Crème vintage
        label.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    setupLabel(attackLabel, "ATTACK");
    setupLabel(decayLabel, "DECAY");
    setupLabel(sustainLabel, "SUSTAIN");
    setupLabel(releaseLabel, "RELEASE");
    setupLabel(cutoffLabel, "CUTOFF");
    setupLabel(resonanceLabel, "RESONANCE");
    setupLabel(filterEnvAmountLabel, "ENV AMT");
    setupLabel(waveformLabel, "WAVEFORM");
    setupLabel(voicesLabel, "VOICES");
    setupLabel(detuneLabel, "DETUNE");
    setupLabel(stereoLabel, "STEREO");

    setupLabel(filterAttackLabel, "ATTACK");
    setupLabel(filterDecayLabel, "DECAY");
    setupLabel(filterSustainLabel, "SUSTAIN");
    setupLabel(filterReleaseLabel, "RELEASE");

    // Labels NOISE
    setupLabel(noiseLevelLabel, "LEVEL");

    // Titres de sections vintage (orange chaud)
    auto setupSectionLabel = [this](juce::Label& label, const juce::String& text, juce::Colour colour)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, colour);
        label.setFont(juce::Font(17.0f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    setupSectionLabel(adsrSectionLabel, "AMP ENVELOPE", juce::Colour(0xffff8c42));      // Orange vintage
    setupSectionLabel(filterAdsrSectionLabel, "FILTER ENVELOPE", juce::Colour(0xffff8c42)); // Orange vintage
    setupSectionLabel(filterSectionLabel, "FILTER", juce::Colour(0xff00ff88));         // Vert
    setupSectionLabel(oscSectionLabel, "OSCILLATOR", juce::Colour(0xffd4af37));        // Doré
    setupSectionLabel(unisonSectionLabel, "UNISON", juce::Colour(0xff00d4ff));         // Cyan
    setupSectionLabel(noiseSectionLabel, "NOISE", juce::Colour(0xffff6b9d));           // Rose (NOUVEAU!)

    // ================= Création des attachements =================

    // ADSR
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "attack", attackKnob);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "decay", decayKnob);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "sustain", sustainKnob);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "release", releaseKnob);

    // Filtre
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "cutoff", cutoffKnob);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "resonance", resonanceKnob);
    filterEnvAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filterEnvAmount", filterEnvAmountKnob);

    // ADSR du filtre (NOUVEAU!)
    filterAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filterAttack", filterAttackKnob);
    filterDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filterDecay", filterDecayKnob);
    filterSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filterSustain", filterSustainKnob);
    filterReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "filterRelease", filterReleaseKnob);

    // Oscillateur
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "waveform", waveformSelector);

    // Unison
    voicesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "voices", voicesKnob);
    detuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "detune", detuneKnob);
    stereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "stereo", stereoKnob);

    // NOISE (NOUVEAU!)
    noiseEnableAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), "noiseEnable", noiseEnableButton);
    noiseLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "noiseLevel", noiseLevelKnob);

    // 📐 Taille de la fenêtre (interface compacte optimisée)
    setSize(1070, 680);
}

// ================= Destructeur =================
SYNTH_1AudioProcessorEditor::~SYNTH_1AudioProcessorEditor()
{
    // 🔧 Important : restaurer le LookAndFeel par défaut
    // 📝 Explication : Évite les crashes si le LookAndFeel est détruit avant les composants
    setLookAndFeel(nullptr);
}

// ================= Rendu graphique =================
void SYNTH_1AudioProcessorEditor::paint(juce::Graphics& g)
{
    // 🌲 Fond bois vintage (inspiré Moog Minimoog)
    juce::ColourGradient woodGradient(
        juce::Colour(0xff4a3728),  // Brun chaud en haut
        0.0f, 0.0f,
        juce::Colour(0xff2d1f17),  // Brun foncé en bas
        0.0f, (float)getHeight(),
        false);
    g.setGradientFill(woodGradient);
    g.fillAll();

    // Effet texture bois (lignes horizontales subtiles)
    g.setColour(juce::Colour(0xff3a2818).withAlpha(0.3f));
    for (int i = 0; i < getHeight(); i += 8)
    {
        g.fillRect(0, i, getWidth(), 2);
    }

    // 📦 Panneaux vintage en métal brossé
    auto drawVintagePanel = [&g](int x, int y, int width, int height, juce::Colour accentColour)
    {
        auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height);

        // Fond métal sombre
        juce::ColourGradient metalGradient(
            juce::Colour(0xff2a2a2a), bounds.getX(), bounds.getY(),
            juce::Colour(0xff1a1a1a), bounds.getX(), bounds.getBottom(),
            false);
        g.setGradientFill(metalGradient);
        g.fillRoundedRectangle(bounds, 6.0f);

        // Biseautage (effet 3D)
        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRoundedRectangle(bounds.reduced(1), 6.0f, 2.0f);

        // Bordure accentuée orange vintage
        g.setColour(accentColour);
        g.drawRoundedRectangle(bounds.reduced(3), 5.0f, 2.5f);

        // Vis aux coins (détail authentique)
        auto drawScrew = [&g](float cx, float cy)
        {
            g.setColour(juce::Colour(0xff6a6a6a));
            g.fillEllipse(cx - 4, cy - 4, 8, 8);
            g.setColour(juce::Colour(0xff4a4a4a));
            g.drawLine(cx - 3, cy, cx + 3, cy, 1.5f);
        };

        drawScrew(bounds.getX() + 10, bounds.getY() + 10);
        drawScrew(bounds.getRight() - 10, bounds.getY() + 10);
        drawScrew(bounds.getX() + 10, bounds.getBottom() - 10);
        drawScrew(bounds.getRight() - 10, bounds.getBottom() - 10);
    };

    // Panneaux réorganisés selon la nouvelle logique
    drawVintagePanel(15, 20, 240, 185, juce::Colour(0xffff8c42));   // AMP ENVELOPE (orange)
    drawVintagePanel(265, 20, 240, 185, juce::Colour(0xffff8c42));  // FILTER ENVELOPE (orange)
    drawVintagePanel(515, 20, 235, 185, juce::Colour(0xff00d4ff));  // UNISON (cyan)
    drawVintagePanel(760, 20, 295, 185, juce::Colour(0xffd4af37));  // OSCILLATOR (doré, même hauteur que unison)
    drawVintagePanel(265, 215, 490, 185, juce::Colour(0xff00ff88)); // FILTER (vert)
    drawVintagePanel(760, 215, 295, 185, juce::Colour(0xffff6b9d)); // NOISE (rose, en dessous oscillator, même hauteur que filter)

    // Label pour l'analyseur
    g.setColour(juce::Colour(0xfff4e6d8));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawText("SPECTRUM ANALYZER", 165, 406, 745, 30, juce::Justification::centred);

    // 🎹 Logo vintage à gauche (panneau séparé)
    // Fond panneau vintage pour le logo
    auto logoBounds = juce::Rectangle<float>(15.0f, 215.0f, 240.0f, 185.0f);

    // Fond métal sombre
    juce::ColourGradient metalGradient(
        juce::Colour(0xff2a2a2a), logoBounds.getX(), logoBounds.getY(),
        juce::Colour(0xff1a1a1a), logoBounds.getX(), logoBounds.getBottom(),
        false);
    g.setGradientFill(metalGradient);
    g.fillRoundedRectangle(logoBounds, 6.0f);

    // Biseautage
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(logoBounds.reduced(1), 6.0f, 2.0f);

    // Bordure orange
    g.setColour(juce::Colour(0xffff8c42));
    g.drawRoundedRectangle(logoBounds.reduced(3), 5.0f, 2.5f);

    // Vis aux coins
    auto drawScrew = [&g](float cx, float cy)
    {
        g.setColour(juce::Colour(0xff6a6a6a));
        g.fillEllipse(cx - 4, cy - 4, 8, 8);
        g.setColour(juce::Colour(0xff4a4a4a));
        g.drawLine(cx - 3, cy, cx + 3, cy, 1.5f);
    };

    drawScrew(logoBounds.getX() + 10, logoBounds.getY() + 10);
    drawScrew(logoBounds.getRight() - 10, logoBounds.getY() + 10);
    drawScrew(logoBounds.getX() + 10, logoBounds.getBottom() - 10);
    drawScrew(logoBounds.getRight() - 10, logoBounds.getBottom() - 10);

    // Logo centré avec ombre
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.setFont(juce::Font(40.0f, juce::Font::bold));
    g.drawText("LULU", 15, 255, 240, 40, juce::Justification::centred);

    g.setColour(juce::Colour(0xfff4e6d8));
    g.setFont(juce::Font(40.0f, juce::Font::bold));
    g.drawText("LULU", 15, 253, 240, 40, juce::Justification::centred);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.setFont(juce::Font(40.0f, juce::Font::bold));
    g.drawText("SYNTH", 15, 300, 240, 40, juce::Justification::centred);

    g.setColour(juce::Colour(0xfff4e6d8));
    g.setFont(juce::Font(40.0f, juce::Font::bold));
    g.drawText("SYNTH", 15, 298, 240, 40, juce::Justification::centred);

    // Sous-titre centré
    g.setColour(juce::Colour(0xffff8c42));
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    g.drawText("ANALOG SYNTHESIZER", 15, 350, 240, 20, juce::Justification::centred);
}

// ================= Positionnement des composants =================
void SYNTH_1AudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // ================= Analyseur de spectre =================
    spectrumAnalyzer.setBounds(15, 440, getWidth() - 30, 100);

    // ================= Clavier MIDI (en bas, pleine largeur) =================
    keyboardComponent.setBounds(15, 570, getWidth() - 30, 90);

    // ================= Titres de sections (réorganisés) =================
    adsrSectionLabel.setBounds(25, 25, 220, 30);         // AMP ENVELOPE
    filterAdsrSectionLabel.setBounds(275, 25, 220, 30);  // FILTER ENVELOPE
    unisonSectionLabel.setBounds(525, 25, 215, 30);      // UNISON
    oscSectionLabel.setBounds(770, 25, 275, 30);         // OSCILLATOR (même hauteur que les autres)
    filterSectionLabel.setBounds(275, 220, 470, 30);     // FILTER
    noiseSectionLabel.setBounds(770, 220, 275, 30);      // NOISE (en dessous oscillator)

    // ================= Dimensions des contrôles (UNIFORMISÉES!) =================
    int knobSize = 90;        // Taille UNIQUE pour TOUS les knobs rotatifs (60x60px)
    int sliderWidth = 40;     // Largeur des sliders verticaux ADSR
    int sliderHeight = 100;   // Hauteur des sliders verticaux ADSR
    int knobY = 80;           // Position Y des knobs (ajustée pour 60px)
    int sliderY = 65;         // Position Y des sliders ADSR

    // ================= AMP ENVELOPE (4 sliders verticaux) =================
    attackLabel.setBounds(25, 55, sliderWidth, 18);
    attackKnob.setBounds(25, sliderY, sliderWidth, sliderHeight);

    decayLabel.setBounds(75, 55, sliderWidth, 18);
    decayKnob.setBounds(75, sliderY, sliderWidth, sliderHeight);

    sustainLabel.setBounds(125, 55, sliderWidth, 18);
    sustainKnob.setBounds(125, sliderY, sliderWidth, sliderHeight);

    releaseLabel.setBounds(175, 55, sliderWidth, 18);
    releaseKnob.setBounds(175, sliderY, sliderWidth, sliderHeight);

    // ================= FILTER ENVELOPE (4 sliders verticaux) =================
    filterAttackLabel.setBounds(285, 55, sliderWidth, 18);
    filterAttackKnob.setBounds(285, sliderY, sliderWidth, sliderHeight);

    filterDecayLabel.setBounds(335, 55, sliderWidth, 18);
    filterDecayKnob.setBounds(335, sliderY, sliderWidth, sliderHeight);

    filterSustainLabel.setBounds(385, 55, sliderWidth, 18);
    filterSustainKnob.setBounds(385, sliderY, sliderWidth, sliderHeight);

    filterReleaseLabel.setBounds(435, 55, sliderWidth, 18);
    filterReleaseKnob.setBounds(435, sliderY, sliderWidth, sliderHeight);

    // ================= UNISON (3 knobs centrés, 60px) =================
    // Panel UNISON: x=515, width=235, 3 knobs de 60px uniformisés
    int unisonStartX = 523;  // Ajusté pour centrage avec knobs 60px
    int unisonSpacing = 70;  // Espacement entre les knobs

    voicesLabel.setBounds(unisonStartX, 55, knobSize, 18);
    voicesKnob.setBounds(unisonStartX, knobY, knobSize, knobSize);

    detuneLabel.setBounds(unisonStartX + unisonSpacing, 55, knobSize, 18);
    detuneKnob.setBounds(unisonStartX + unisonSpacing, knobY, knobSize, knobSize);

    stereoLabel.setBounds(unisonStartX + unisonSpacing * 2, 55, knobSize, 18);
    stereoKnob.setBounds(unisonStartX + unisonSpacing * 2, knobY, knobSize, knobSize);
 
    // ================= OSCILLATOR (ComboBox centré verticalement) =================
    waveformLabel.setBounds(770, 55, 275, 18);
    waveformSelector.setBounds(770, 85, 275, 40);

    // ================= FILTER (3 knobs centrés, 60px uniformisés) =================
    // Panel FILTER: x=265, width=490, 3 knobs de 60px
    int filterStartX = 330;  // Ajusté pour centrage avec knobs 60px
    int filterSpacing = 90;  // Espacement entre les knobs

    cutoffLabel.setBounds(filterStartX, 250, knobSize, 18);
    cutoffKnob.setBounds(filterStartX, 280, knobSize, knobSize);

    resonanceLabel.setBounds(filterStartX + filterSpacing, 250, knobSize, 18);
    resonanceKnob.setBounds(filterStartX + filterSpacing, 280, knobSize, knobSize);
 
    filterEnvAmountLabel.setBounds(filterStartX + filterSpacing * 2, 250, knobSize, 18);
    filterEnvAmountKnob.setBounds(filterStartX + filterSpacing * 2, 280, knobSize, knobSize);

    // ================= NOISE (toggle button + knob centrés, 60px uniformisé) =================
    // Panel NOISE: x=760, width=295, centré verticalement
    noiseEnableButton.setBounds(780, 255, 100, 30);       // Toggle button ON/OFF
    noiseLevelLabel.setBounds(900, 253, knobSize, 18);    // Label LEVEL (même largeur que knob)
    noiseLevelKnob.setBounds(900, 275, knobSize, knobSize);  // Knob 60px uniformisé
}
