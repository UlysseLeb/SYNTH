/*
  ==============================================================================

    PluginEditor.h

    RÔLE : Définit l'interface graphique (GUI) du plugin

    CONCEPT :
    - AudioProcessorEditor = classe de base JUCE pour les interfaces de plugins
    - Contient tous les composants visuels (sliders, boutons, clavier...)
    - Se synchronise automatiquement avec le processeur audio

    ARCHITECTURE :
    - Composants visuels (Slider, MidiKeyboardComponent...)
    - Attachements (lient les sliders aux paramètres du processeur)
    - Méthodes de rendu (paint, resized)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ModernLookAndFeel.h"  // 🎨 Notre style custom
#include "SpectrumAnalyzer.h"   // 📊 Analyseur de spectre

// ================= Classe de l'interface graphique =================
// Hérite de juce::AudioProcessorEditor (classe de base JUCE)
class SYNTH_1AudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    // Constructeur : initialise l'interface avec une référence au processeur
    // explicit = empêche les conversions implicites (bonne pratique C++)
    explicit SYNTH_1AudioProcessorEditor(SYNTH_1AudioProcessor&);

    // Destructeur : nettoie les ressources de l'interface
    ~SYNTH_1AudioProcessorEditor() override;

    // Rendu graphique : dessine l'arrière-plan et les éléments visuels
    // Appelé automatiquement par JUCE quand l'interface doit être redessinée
    void paint(juce::Graphics&) override;

    // Positionnement des composants : définit où chaque élément est affiché
    // Appelé quand la fenêtre est redimensionnée ou à l'initialisation
    void resized() override;

    // Accès à l'analyseur de spectre (pour alimenter depuis le processeur)
    SpectrumAnalyzer& getSpectrumAnalyzer() { return spectrumAnalyzer; }

private:
    // ================= Référence au processeur audio =================

    // audioProcessor : référence au processeur audio principal
    // Permet d'accéder aux paramètres (ADSR, filtre...) et à l'état du clavier
    SYNTH_1AudioProcessor& audioProcessor;

    // ================= Composants visuels =================

    // keyboardComponent : clavier MIDI virtuel interactif
    // Affiche un clavier de piano qui réagit aux clics et aux notes MIDI
    juce::MidiKeyboardComponent keyboardComponent;

    // Knobs ADSR : contrôles rotatifs pour l'enveloppe (NOUVEAU!)
    // Explication : Remplacent les sliders verticaux par des knobs rotatifs
    //    - Plus professionnel (tous les synthés pros utilisent des knobs)
    //    - Plus compact (prend moins d'espace)
    //    - Plus intuitif (comme du vrai hardware)
    juce::Slider attackKnob;    // Temps de montée (Attack)
    juce::Slider decayKnob;     // Temps de descente (Decay)
    juce::Slider sustainKnob;   // Niveau de maintien (Sustain)
    juce::Slider releaseKnob;   // Temps d'extinction (Release)

    // Knobs du filtre : contrôles rotatifs pour le timbre
    juce::Slider cutoffKnob;     // Fréquence de coupure
    juce::Slider resonanceKnob;  // Résonance (Q factor)
    juce::Slider filterEnvAmountKnob;  // Intensité de l'enveloppe du filtre

    // Knobs ADSR du filtre (NOUVEAU!)
    juce::Slider filterAttackKnob;
    juce::Slider filterDecayKnob;
    juce::Slider filterSustainKnob;
    juce::Slider filterReleaseKnob;

    // Knobs Unison : contrôles pour le son épais (NOUVEAU!)
    juce::Slider voicesKnob;     // Nombre de voix (1-7)
    juce::Slider detuneKnob;     // Désaccordage (0-100%)
    juce::Slider stereoKnob;     // Largeur stéréo (0-100%)

    // ComboBox de forme d'onde : menu déroulant pour choisir la forme d'onde
    // Explication : ComboBox = menu déroulant avec plusieurs choix
    //    - Permet de sélectionner entre Sine, Saw, Square, Triangle
    //    - Plus compact qu'un ensemble de boutons radio
    //    - Interface standard dans les synthés professionnels
    juce::ComboBox waveformSelector;

    // Contrôles NOISE (NOUVEAU!)
    // Explication : Générateur de bruit blanc pour enrichir le son
    juce::ToggleButton noiseEnableButton;  // Bouton ON/OFF pour activer le bruit
    juce::Slider noiseLevelKnob;           // Knob pour le niveau du bruit

    // Labels : textes descriptifs pour identifier chaque contrôle
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label cutoffLabel;
    juce::Label resonanceLabel;
    juce::Label waveformLabel;  // Label pour le sélecteur de forme d'onde
    juce::Label voicesLabel;    // Label pour le nombre de voix
    juce::Label detuneLabel;    // Label pour le detune
    juce::Label stereoLabel;    // Label pour la largeur stéréo

    juce::Label filterAttackLabel;
    juce::Label filterDecayLabel;
    juce::Label filterSustainLabel;
    juce::Label filterReleaseLabel;
    juce::Label filterEnvAmountLabel;

    juce::Label adsrSectionLabel;        // Titre de section "ENVELOPPE"
    juce::Label filterAdsrSectionLabel;  // Titre de section "FILTER ENV"
    juce::Label filterSectionLabel;      // Titre de section "FILTRE"
    juce::Label oscSectionLabel;         // Titre de section "OSCILLATEUR"
    juce::Label unisonSectionLabel;      // Titre de section "UNISON"
    juce::Label noiseSectionLabel;       // Titre de section "NOISE" (NOUVEAU!)
    juce::Label noiseLevelLabel;         // Label pour le niveau du bruit

    // Analyseur de spectre (NOUVEAU!)
    // Explication : Affichage temps réel des fréquences
    //    - Comme dans Serum, Vital, Phase Plant
    //    - Visualisation professionnelle du son
    SpectrumAnalyzer spectrumAnalyzer;

    // ================= Attachements (liaisons paramètres ↔ sliders) =================

    // Attachements : lient automatiquement les sliders aux paramètres du processeur
    // Quand le slider bouge → le paramètre change
    // Quand le paramètre change (automation, preset...) → le slider bouge
    // std::unique_ptr = pointeur intelligent (gestion mémoire automatique)

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterEnvAmountAttachment;

    // Attachements ADSR du filtre (NOUVEAU!)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterReleaseAttachment;

    // Attachement pour la forme d'onde
    // Explication : ComboBoxAttachment lie le menu déroulant au paramètre
    //    - Fonctionne comme SliderAttachment mais pour les choix discrets
    //    - Synchronise automatiquement la sélection avec le paramètre
    //    - Supporte l'automation et les presets
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;

    // Attachements Unison (NOUVEAU!)
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> voicesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> detuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoAttachment;

    // Attachements NOISE (NOUVEAU!)
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> noiseEnableAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseLevelAttachment;

    // Style graphique custom (NOUVEAU!)
    // xplication : Notre LookAndFeel personnalisé
    //    - Appliqué à tous les composants
    //    - Donne le style pro moderne
    ModernLookAndFeel modernLookAndFeel;

    //Macro JUCE : empêche la copie de l'éditeur (sécurité)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SYNTH_1AudioProcessorEditor)
};


