/*
  ==============================================================================

    PluginEditor.cpp

    📌 IMPLÉMENTATION de l'interface graphique du plugin
    Contient toute la logique d'initialisation et de rendu de l'UI

  ==============================================================================
*/

#include "PluginEditor.h"

// ================= Constructeur =================
// 🏗️ Appelé quand l'utilisateur ouvre la fenêtre du plugin
SYNTH_1AudioProcessorEditor::SYNTH_1AudioProcessorEditor(SYNTH_1AudioProcessor& p)
    // 🔗 Initialisation de la classe de base
    : AudioProcessorEditor(&p),  // Passe un pointeur vers le processeur
      audioProcessor(p),          // Stocke une référence au processeur
      // 🎹 Initialisation du clavier MIDI virtuel
      // Paramètres :
      //   - p.getKeyboardState() : état du clavier (notes enfoncées)
      //   - horizontalKeyboard : orientation horizontale
      keyboardComponent(p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    // ================= Configuration du clavier MIDI =================

    // 👀 Rendre le clavier visible et l'ajouter à l'interface
    addAndMakeVisible(keyboardComponent);

    // ================= Configuration des sliders ADSR =================

    // 📈 ATTACK SLIDER
    attackSlider.setSliderStyle(juce::Slider::LinearVertical);  // Slider vertical
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);  // Boîte de texte en bas
    attackSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightblue);  // Curseur bleu clair
    attackSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkblue);   // Piste bleu foncé
    addAndMakeVisible(attackSlider);  // Rendre visible

    // 📉 DECAY SLIDER
    decaySlider.setSliderStyle(juce::Slider::LinearVertical);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    decaySlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightgreen);
    decaySlider.setColour(juce::Slider::trackColourId, juce::Colours::darkgreen);
    addAndMakeVisible(decaySlider);

    // 🔊 SUSTAIN SLIDER
    sustainSlider.setSliderStyle(juce::Slider::LinearVertical);
    sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    sustainSlider.setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
    sustainSlider.setColour(juce::Slider::trackColourId, juce::Colours::orange);
    addAndMakeVisible(sustainSlider);

    // 📉 RELEASE SLIDER
    releaseSlider.setSliderStyle(juce::Slider::LinearVertical);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    releaseSlider.setColour(juce::Slider::thumbColourId, juce::Colours::lightcoral);
    releaseSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkred);
    addAndMakeVisible(releaseSlider);

    // ================= Configuration des sliders du filtre =================

    // 🎚️ CUTOFF SLIDER (fréquence de coupure)
    cutoffSlider.setSliderStyle(juce::Slider::LinearVertical);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    cutoffSlider.setColour(juce::Slider::thumbColourId, juce::Colours::cyan);
    cutoffSlider.setColour(juce::Slider::trackColourId, juce::Colours::darkturquoise);
    addAndMakeVisible(cutoffSlider);

    // 🔊 RESONANCE SLIDER (résonance du filtre)
    resonanceSlider.setSliderStyle(juce::Slider::LinearVertical);
    resonanceSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    resonanceSlider.setColour(juce::Slider::thumbColourId, juce::Colours::violet);
    resonanceSlider.setColour(juce::Slider::trackColourId, juce::Colours::purple);
    addAndMakeVisible(resonanceSlider);

    // ================= Configuration du sélecteur de forme d'onde =================

    // 🎵 WAVEFORM SELECTOR (menu déroulant)
    // 📝 Explication : Configuration du ComboBox
    //    - addItem() ajoute un choix dans le menu
    //    - Le nombre après le texte = ID unique (DOIT commencer à 1, pas 0!)
    //    - L'ordre doit correspondre à l'enum OscillatorWaveform
    waveformSelector.addItem("Sine", 1);      // ID 1 = index 0
    waveformSelector.addItem("Saw", 2);       // ID 2 = index 1
    waveformSelector.addItem("Square", 3);    // ID 3 = index 2
    waveformSelector.addItem("Triangle", 4);  // ID 4 = index 3
    waveformSelector.setSelectedId(1);  // Sélection par défaut : Sine
    waveformSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff2a2a3e));
    waveformSelector.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    waveformSelector.setColour(juce::ComboBox::outlineColourId, juce::Colours::lightblue);
    addAndMakeVisible(waveformSelector);

    // ================= Configuration des labels =================

    // 🏷️ Helper lambda pour configurer un label
    auto setupLabel = [this](juce::Label& label, const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        label.setFont(juce::Font(14.0f, juce::Font::bold));
        addAndMakeVisible(label);
    };

    // Labels pour les sliders ADSR
    setupLabel(attackLabel, "ATTACK");
    setupLabel(decayLabel, "DECAY");
    setupLabel(sustainLabel, "SUSTAIN");
    setupLabel(releaseLabel, "RELEASE");

    // Labels pour les sliders du filtre
    setupLabel(cutoffLabel, "CUTOFF");
    setupLabel(resonanceLabel, "RESONANCE");

    // Labels pour l'oscillateur
    setupLabel(waveformLabel, "WAVEFORM");

    // Titres de sections (plus grands)
    // 📝 Explication : Les titres de sections organisent visuellement l'interface
    //    - Police plus grande (18pt) pour la hiérarchie visuelle
    //    - Couleurs différentes pour identifier les sections rapidement
    //    - Centrage pour un aspect professionnel
    adsrSectionLabel.setText("ENVELOPPE", juce::dontSendNotification);
    adsrSectionLabel.setJustificationType(juce::Justification::centred);
    adsrSectionLabel.setColour(juce::Label::textColourId, juce::Colours::lightblue);
    adsrSectionLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(adsrSectionLabel);

    filterSectionLabel.setText("FILTRE", juce::dontSendNotification);
    filterSectionLabel.setJustificationType(juce::Justification::centred);
    filterSectionLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    filterSectionLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(filterSectionLabel);

    oscSectionLabel.setText("OSCILLATEUR", juce::dontSendNotification);
    oscSectionLabel.setJustificationType(juce::Justification::centred);
    oscSectionLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    oscSectionLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    addAndMakeVisible(oscSectionLabel);

    // ================= Création des attachements (liaisons) =================

    // 🔗 Attacher les sliders du filtre aux paramètres du processeur
    // std::make_unique = crée un pointeur intelligent (gestion mémoire auto)
    // Paramètres :
    //   - audioProcessor.getValueTreeState() : arbre de paramètres
    //   - "cutoff" : nom du paramètre
    //   - cutoffSlider : slider à lier
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "cutoff", cutoffSlider);

    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "resonance", resonanceSlider);

    // 🔗 Attacher le sélecteur de forme d'onde au paramètre
    // 📝 Explication : ComboBoxAttachment synchronise le menu avec le paramètre
    //    - Quand on choisit une forme d'onde → le paramètre change
    //    - Quand le paramètre change (automation, preset) → le menu se met à jour
    //    - Le ComboBox ID doit correspondre à parameterID + 1 (car IDs commencent à 1)
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), "waveform", waveformSelector);

    // 🔗 Attacher les sliders ADSR aux paramètres du processeur
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), "release", releaseSlider);

    // 📐 Définir la taille de la fenêtre (largeur × hauteur en pixels)
    // 📝 Explication : Taille ajustée pour accueillir :
    //    - Le clavier MIDI virtuel en haut
    //    - La section ADSR (gauche)
    //    - La section FILTRE (centre)
    //    - La section OSCILLATEUR (droite)
    setSize(600, 340);  // Élargie à 600px pour la nouvelle section
}

// ================= Destructeur =================
// 🧹 Appelé quand l'utilisateur ferme la fenêtre du plugin
// Rien à nettoyer manuellement : JUCE gère tout automatiquement
SYNTH_1AudioProcessorEditor::~SYNTH_1AudioProcessorEditor() {}

// ================= Rendu graphique =================
// 🎨 Appelé automatiquement par JUCE pour dessiner l'interface
// Se déclenche quand :
//   - La fenêtre est ouverte
//   - La fenêtre est redimensionnée
//   - Un composant demande un rafraîchissement (repaint())
void SYNTH_1AudioProcessorEditor::paint(juce::Graphics& g)
{
    // 🌈 Créer un beau dégradé pour l'arrière-plan
    juce::ColourGradient gradient(
        juce::Colour(0xff1a1a2e),  // Bleu foncé en haut
        0.0f, 0.0f,
        juce::Colour(0xff16213e),  // Bleu marine en bas
        0.0f, (float)getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillAll();

    // 📦 Dessiner les zones de section avec des bordures
    // 📝 Explication : Les rectangles arrondis créent une séparation visuelle
    //    - Fond semi-transparent (0x22ffffff) pour la profondeur
    //    - Bordure colorée pour identifier rapidement chaque section
    //    - Coins arrondis (10px) pour un look moderne
    auto bounds = getLocalBounds();

    // Zone ADSR (gauche)
    g.setColour(juce::Colour(0x22ffffff));  // Blanc semi-transparent
    g.fillRoundedRectangle(10.0f, 140.0f, 270.0f, 190.0f, 10.0f);
    g.setColour(juce::Colours::lightblue);
    g.drawRoundedRectangle(10.0f, 140.0f, 270.0f, 190.0f, 10.0f, 2.0f);

    // Zone Filtre (centre)
    g.setColour(juce::Colour(0x22ffffff));
    g.fillRoundedRectangle(290.0f, 140.0f, 140.0f, 190.0f, 10.0f);
    g.setColour(juce::Colours::cyan);
    g.drawRoundedRectangle(290.0f, 140.0f, 140.0f, 190.0f, 10.0f, 2.0f);

    // Zone Oscillateur (droite)
    g.setColour(juce::Colour(0x22ffffff));
    g.fillRoundedRectangle(440.0f, 140.0f, 150.0f, 190.0f, 10.0f);
    g.setColour(juce::Colours::orange);
    g.drawRoundedRectangle(440.0f, 140.0f, 150.0f, 190.0f, 10.0f, 2.0f);

    // 🎹 Titre principal
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(24.0f, juce::Font::bold));
    g.drawText("SYNTH_1", bounds.removeFromTop(130).reduced(10),
               juce::Justification::topLeft, true);
}

// ================= Positionnement des composants =================
// 📐 Appelé automatiquement par JUCE pour positionner les éléments de l'UI
// Se déclenche quand :
//   - La fenêtre est ouverte
//   - La fenêtre est redimensionnée
void SYNTH_1AudioProcessorEditor::resized()
{
    // 📦 Récupère la zone totale de la fenêtre (Rectangle<int>)
    auto area = getLocalBounds();

    // 📏 Définir les dimensions
    auto keyboardHeight = 100;
    auto sliderWidth = 60;
    auto sliderHeight = 120;
    auto labelHeight = 20;

    // ================= Positionnement du clavier MIDI =================
    keyboardComponent.setBounds(area.removeFromTop(keyboardHeight).reduced(10));

    // ================= Titres de sections =================
    // 📝 Explication : Positionnement des titres de sections
    //    - Y = 145 : juste en dessous du clavier MIDI
    //    - Largeur adaptée au contenu de chaque section
    adsrSectionLabel.setBounds(20, 145, 250, 25);
    filterSectionLabel.setBounds(300, 145, 120, 25);
    oscSectionLabel.setBounds(450, 145, 130, 25);

    // ================= SECTION ADSR (gauche) =================
    int adsrY = 180;  // Position verticale de départ
    int adsrSpacing = 65;  // Espacement entre les sliders

    // Labels ADSR (au-dessus des sliders)
    attackLabel.setBounds(20, adsrY, sliderWidth, labelHeight);
    decayLabel.setBounds(20 + adsrSpacing, adsrY, sliderWidth, labelHeight);
    sustainLabel.setBounds(20 + adsrSpacing * 2, adsrY, sliderWidth, labelHeight);
    releaseLabel.setBounds(20 + adsrSpacing * 3, adsrY, sliderWidth, labelHeight);

    // Sliders ADSR
    attackSlider.setBounds(20, adsrY + labelHeight + 5, sliderWidth, sliderHeight);
    decaySlider.setBounds(20 + adsrSpacing, adsrY + labelHeight + 5, sliderWidth, sliderHeight);
    sustainSlider.setBounds(20 + adsrSpacing * 2, adsrY + labelHeight + 5, sliderWidth, sliderHeight);
    releaseSlider.setBounds(20 + adsrSpacing * 3, adsrY + labelHeight + 5, sliderWidth, sliderHeight);

    // ================= SECTION FILTRE (droite) =================
    int filterY = 180;
    int filterX = 300;
    int filterSpacing = 65;

    // Labels Filtre (au-dessus des sliders)
    cutoffLabel.setBounds(filterX, filterY, sliderWidth, labelHeight);
    resonanceLabel.setBounds(filterX + filterSpacing, filterY, sliderWidth, labelHeight);

    // Sliders Filtre
    cutoffSlider.setBounds(filterX, filterY + labelHeight + 5, sliderWidth, sliderHeight);
    resonanceSlider.setBounds(filterX + filterSpacing, filterY + labelHeight + 5, sliderWidth, sliderHeight);

    // ================= SECTION OSCILLATEUR (droite) =================
    // 📝 Explication : Positionnement du sélecteur de forme d'onde
    //    - ComboBox centré horizontalement dans la zone oscillateur
    //    - Largeur de 120px pour accueillir "Triangle" (le plus long)
    //    - Position verticale alignée avec les autres contrôles
    int oscX = 450;
    int oscY = 180;

    // Label Waveform
    waveformLabel.setBounds(oscX, oscY, 120, labelHeight);

    // ComboBox Waveform (menu déroulant)
    waveformSelector.setBounds(oscX, oscY + labelHeight + 5, 120, 30);
}

