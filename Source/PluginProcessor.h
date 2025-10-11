/*
  ==============================================================================

    PluginProcessor.h

    📌 RÔLE : Cerveau du plugin - gère TOUT le traitement audio et MIDI

    🎯 HIÉRARCHIE JUCE :
    AudioProcessor (classe de base JUCE)
        ↓
    SYNTH_1AudioProcessor (notre classe)
        ↓ contient
    Synthesiser (moteur JUCE qui gère les voix)
        ↓ contient
    8 × SynthVoice (nos générateurs de son)

    💡 FLUX DE DONNÉES :
    1. MIDI arrive → processBlock()
    2. Synthesiser distribue les notes aux voix disponibles
    3. Chaque voix génère son audio dans renderNextBlock()
    4. Le tout est mixé et envoyé vers la sortie audio

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// ================= Classe principale du processeur audio =================
// Hérite de juce::AudioProcessor (interface standard des plugins audio)
class SYNTH_1AudioProcessor : public juce::AudioProcessor
{
public:
    // 🏗️ Constructeur / Destructeur
    SYNTH_1AudioProcessor();
    ~SYNTH_1AudioProcessor() override;

    // ================= Accesseurs publics =================

    // 📦 Retourne l'arbre de paramètres (pour l'éditeur)
    // Permet à l'interface graphique d'accéder aux sliders (attack, decay, etc.)
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

    // 🎹 Retourne l'état du clavier MIDI
    // Permet d'afficher le clavier virtuel dans l'interface
    juce::MidiKeyboardState& getKeyboardState() noexcept { return keyboardState; }

    // 📊 Récupère les paramètres ADSR actuels
    // ⚡ Appelé à chaque bloc audio pour synchroniser les voix
    juce::ADSR::Parameters getADSRParams() const
    {
        juce::ADSR::Parameters params;
        // Récupère les valeurs des sliders depuis l'arbre de paramètres
        // getRawParameterValue() retourne un pointeur → on déréférence avec *
        params.attack  = *parameters.getRawParameterValue("attack");
        params.decay   = *parameters.getRawParameterValue("decay");
        params.sustain = *parameters.getRawParameterValue("sustain");
        params.release = *parameters.getRawParameterValue("release");
        return params;
    }

    // 🎚️ Récupère les paramètres ADSR du filtre (NOUVEAU!)
    // ⚡ Similaire à getADSRParams() mais pour le filtre
    juce::ADSR::Parameters getFilterADSRParams() const
    {
        juce::ADSR::Parameters params;
        params.attack  = *parameters.getRawParameterValue("filterAttack");
        params.decay   = *parameters.getRawParameterValue("filterDecay");
        params.sustain = *parameters.getRawParameterValue("filterSustain");
        params.release = *parameters.getRawParameterValue("filterRelease");
        return params;
    }

    // 🎚️ Structure pour regrouper les paramètres du filtre
    // Simplifie le passage de paramètres (au lieu de 3 floats séparés)
    struct FilterParams {
        float cutoff;      // Fréquence de coupure (Hz)
        float resonance;   // Résonance (Q factor)
        float envAmount;   // Intensité de l'enveloppe (-100 à +100)
    };

    // 🔧 Récupère les paramètres du filtre actuels
    FilterParams getFilterParams() const
    {
        return {
            *parameters.getRawParameterValue("cutoff"),
            *parameters.getRawParameterValue("resonance"),
            *parameters.getRawParameterValue("filterEnvAmount")
        };
    }


    // ================= Méthodes du cycle de vie audio (obligatoires) =================

    // 🎬 Préparation avant le traitement audio
    // Appelé quand le DAW démarre ou change de configuration audio
    // 📝 Paramètres :
    //    - sampleRate : fréquence d'échantillonnage (ex: 44100 Hz)
    //    - samplesPerBlock : taille du buffer audio (ex: 512 samples)
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    // 🧹 Libération des ressources audio
    // Appelé quand le DAW arrête ou met en pause
    void releaseResources() override;

    // 🔌 Configuration des bus audio (entrées/sorties)
    // Vérifie si le plugin supporte une configuration donnée
    // Ex: stéréo out, mono out, etc.
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    // ⚡ CŒUR DU TRAITEMENT AUDIO (méthode la plus importante !)
    // Appelé en boucle pour traiter l'audio et le MIDI
    // 📝 Paramètres :
    //    - buffer : contient l'audio (à remplir pour un synthé)
    //    - midiMessages : contient les événements MIDI (note on/off, CC, etc.)
    // 🔥 Fonction temps-réel : doit être ULTRA rapide (pas d'allocation mémoire !)
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    // ================= Méthodes de l'éditeur (interface graphique) =================

    // 🖥️ Crée l'interface graphique du plugin
    juce::AudioProcessorEditor* createEditor() override;

    // ❓ Le plugin a-t-il une interface graphique ?
    bool hasEditor() const override;

    // ================= Informations sur le plugin =================

    // 📝 Nom du plugin
    const juce::String getName() const override;

    // 🎹 Le plugin accepte-t-il le MIDI en entrée ?
    bool acceptsMidi() const override;

    // 🎵 Le plugin produit-il du MIDI en sortie ?
    bool producesMidi() const override;

    // 🎛️ Le plugin est-il un effet MIDI (sans audio) ?
    bool isMidiEffect() const override;

    // ⏱️ Durée de la "queue" audio après arrêt (ex: reverb)
    // Notre synthé s'arrête immédiatement après la release → 0.0
    double getTailLengthSeconds() const override;

    // ================= Gestion des presets (non implémenté) =================

    // 📦 Nombre de presets disponibles
    int getNumPrograms() override;

    // 🎯 Index du preset actuel
    int getCurrentProgram() override;

    // 🔄 Charger un preset par index
    void setCurrentProgram(int index) override;

    // 📝 Nom d'un preset
    const juce::String getProgramName(int index) override;

    // ✏️ Renommer un preset
    void changeProgramName(int index, const juce::String& newName) override;

    // ================= Sauvegarde / Restauration de l'état =================

    // 💾 Sauvegarder l'état du plugin (paramètres) dans un bloc mémoire
    // Appelé quand on sauvegarde un projet DAW
    void getStateInformation(juce::MemoryBlock& destData) override;

    // 📂 Restaurer l'état du plugin depuis un bloc mémoire
    // Appelé quand on ouvre un projet DAW
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    // ================= Membres privés =================

    // 🎹 synth : moteur principal du synthétiseur (classe JUCE)
    //    Gère la polyphonie (distribution des notes aux voix)
    //    Mixe l'audio de toutes les voix actives
    juce::Synthesiser synth;

    // 🎼 keyboardState : état du clavier MIDI virtuel
    //    Suit quelles touches sont enfoncées/relâchées
    //    Utilisé pour l'affichage du clavier dans l'interface
    juce::MidiKeyboardState keyboardState;

    // 🎛️ parameters : arbre de paramètres (ADSR, filtre, etc.)
    //    Gère automatiquement :
    //    • Synchronisation avec l'interface graphique
    //    • Sauvegarde/restauration de l'état
    //    • Automation dans le DAW
    juce::AudioProcessorValueTreeState parameters;

    // 🏗️ Fonction statique pour créer la structure des paramètres
    // Appelée dans le constructeur pour initialiser l'arbre
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // 🚫 Macro JUCE : empêche la copie du processeur (sécurité)
    // Un processeur audio ne doit jamais être copié (ressources uniques)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SYNTH_1AudioProcessor)
};

