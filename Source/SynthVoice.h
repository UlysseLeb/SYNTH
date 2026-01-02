/*
  ==============================================================================

    SynthVoice.h

     RÔLE : Définit une "voix" du synthétiseur - l'unité qui génère le son

     CONCEPT :
    - Une "voix" = une note jouée à un instant T
    - Le synthétiseur a 8 voix → peut jouer 8 notes simultanément (polyphonie)
    - Chaque voix gère :
        • Génération d'onde sinusoïdale
        • Enveloppe ADSR (Attack, Decay, Sustain, Release)
        • Filtre audio

     GÉNÉRATION DU SON :
    - Onde sinusoïdale : sin(angle) où l'angle augmente à chaque sample
    - Fréquence déterminée par la note MIDI (ex: Do = 261.6 Hz)
    - Amplitude modulée par l'enveloppe ADSR

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "SynthSound.h"
#include "Oscillator.h"        // 📦 Notre classe oscillateur multi-formes d'onde
#include "UnisonOscillator.h"  // 📦 Oscillateur avec Unison (son épais)
#include "VintageProcessor.h"  // 📦 Module de traitement vintage (warmth + drift)

// ================= Classe SynthVoice =================
// Hérite de juce::SynthesiserVoice (classe de base JUCE)
class SynthVoice : public juce::SynthesiserVoice
{
public:
    //  Constructeur : initialise la voix (valeurs par défaut dans private)
    SynthVoice()
    {
        // 🔧 INITIALISER LE FILTRE pour éviter le bruit
        // Type::lowpass = filtre passe-bas (coupe les hautes fréquences)
        filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    //  Question : Cette voix peut-elle jouer ce type de son ?
    //  Vérifie que le son est bien un SynthSound (notre classe custom)
    //  Retourne true si compatible, false sinon
    bool canPlaySound(juce::SynthesiserSound* sound) override;

    //  PRÉPARATION DE LA VOIX
    // Appelé avant le traitement audio pour initialiser les modules DSP
    //  Paramètres :
    //    - sampleRate : fréquence d'échantillonnage (Hz)
    //    - samplesPerBlock : taille du buffer audio
    //    - numChannels : nombre de canaux audio
    void prepareVoice(double sampleRate, int samplesPerBlock, int numChannels)
    {
        //  Configurer le filtre avec le spec audio
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
        spec.numChannels = (juce::uint32)numChannels;
        filter.prepare(spec);

        //  Préparer les ADSR avec le sample rate
        adsr.setSampleRate(sampleRate);
        filterAdsr.setSampleRate(sampleRate);  // NOUVEAU : ADSR du filtre
    }

    //  DÉMARRAGE D'UNE NOTE
    // Appelé quand une touche MIDI est enfoncée (Note On)
    //  Paramètres :
    //    - midiNoteNumber : numéro de la note (0-127, ex: 60 = Do central)
    //    - velocity : force de frappe (0.0-1.0)
    //    - sound : le son à jouer
    //    - currentPitchWheelPosition : position de la molette de pitch
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;

    //  ARRÊT D'UNE NOTE
    // Appelé quand une touche MIDI est relâchée (Note Off)
    //  Paramètres :
    //    - velocity : vitesse de relâchement
    //    - allowTailOff : true = laisser la release de l'ADSR se terminer
    void stopNote(float velocity, bool allowTailOff) override;

    //  ÉVÉNEMENTS MIDI (non utilisés ici, mais obligatoires)
    void pitchWheelMoved(int) override {}      // Molette de pitch (bend)
    void controllerMoved(int, int) override {} // Contrôleurs MIDI (CC)

    //  GÉNÉRATION AUDIO
    // Appelé en boucle pour remplir le buffer audio
    //  Paramètres :
    //    - outputBuffer : buffer audio à remplir
    //    - startSample : position de départ dans le buffer
    //    - numSamples : nombre d'échantillons à générer
    // ⚡ C'est ICI que le son est créé !
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    //  MISE À JOUR DE L'ENVELOPPE ADSR (Amplitude)
    // Appelé depuis le processeur pour synchroniser les paramètres
    //  Explication : L'ADSR contrôle le volume dans le temps
    //    - Attack : temps de montée du son
    //    - Decay : temps pour atteindre le sustain
    //    - Sustain : niveau maintenu tant que la note est jouée
    //    - Release : temps de descente après relâchement
    void updateADSR(const juce::ADSR::Parameters& params) { adsr.setParameters(params); }

    //  MISE À JOUR DE L'ENVELOPPE ADSR DU FILTRE (NOUVEAU!)
    //  Explication : Cette enveloppe module la fréquence de coupure du filtre
    //    - Permet des changements de timbre dynamiques
    //    - Typique des synthés classiques (Moog, ARP...)
    //    - Ex: Attack rapide = son qui s'ouvre (filter sweep)
    void updateFilterADSR(const juce::ADSR::Parameters& params) { filterAdsr.setParameters(params); }

    //  MISE À JOUR DU FILTRE
    // Configure la fréquence de coupure, la résonance et l'intensité de l'enveloppe
    //  Explication : Le filtre modifie le timbre (la couleur du son)
    //    - Cutoff : fréquence au-dessus de laquelle le son est atténué
    //    - Resonance : boost autour de la fréquence de coupure (son nasillard)
    //    - EnvAmount : intensité de la modulation par l'enveloppe (-100 à +100)
    void updateFilter(float cutoff, float resonance, float envAmount);

    //  MISE À JOUR DE LA FORME D'ONDE
    // Change la forme d'onde de l'oscillateur (sine, saw, square, triangle)
    //  Explication : Chaque forme d'onde a un timbre différent
    //    - Sine : son pur, doux (pas d'harmoniques)
    //    - Saw : son brillant, riche (toutes les harmoniques)
    //    - Square : son creux, vintage (harmoniques impaires)
    //    - Triangle : son doux, flûte (harmoniques impaires atténuées)
    void setWaveform(OscillatorWaveform waveform)
    {
        oscillator.setWaveform(waveform);
    }

    //  MISE À JOUR DES PARAMÈTRES UNISON
    // Configure le nombre de voix, détune et largeur stéréo
    //  Explication : L'unison crée un son épais et riche
    //    - voices : 1-7 voix (plus = plus épais)
    //    - detune : 0-1 (écart de fréquence entre les voix)
    //    - stereo : 0-1 (répartition stéréo)
    void updateUnison(int voices, float detune, float stereo)
    {
        oscillator.setNumVoices(voices);
        oscillator.setDetuneAmount(detune);
        oscillator.setStereoWidth(stereo);
    }

    //  MISE À JOUR DES PARAMÈTRES NOISE (NOUVEAU!)
    // Configure l'activation et le niveau du bruit blanc
    //  Explication : Le bruit enrichit le son et ajoute de la texture
    //    - enable : true/false (activer/désactiver le générateur)
    //    - level : 0.0-1.0 (niveau du bruit, 0-100% converti)
    void updateNoise(bool enable, float level)
    {
        noiseEnabled = enable;
        noiseLevel = level / 100.0f;  // Convertir 0-100 en 0.0-1.0
    }


private:
    // ================= Variables pour la génération audio =================

    //  oscillator : générateur d'onde avec Unison
    //  Explication : C'est lui qui crée le son de base
    //    - Peut générer 4 formes d'onde : sine, saw, square, triangle
    //    - Gère la fréquence (note MIDI → Hz)
    //    - UNISON : jusqu'à 7 voix désaccordées pour un son épais
    //    - Sortie STÉRÉO : les voix sont réparties dans l'espace
    UnisonOscillator oscillator;

    //  level : amplitude de base (déterminée par la vélocité MIDI)
    //  Explication : Contrôle le volume global de la voix
    //    - Calculé depuis la vélocité MIDI (force de frappe)
    //    - Multiplié par 0.15 pour éviter la saturation
    //    - Plus vous frappez fort la touche, plus level est élevé
    double level = 0.0;

    //  currentFrequency : fréquence actuelle en Hz
    //  Explication : Stocke la fréquence de la note jouée
    //    - Calculée depuis le numéro de note MIDI (ex: 69 = La 440 Hz)
    //    - Nécessaire pour mettre à jour l'oscillateur
    double currentFrequency = 0.0;

    //  currentSampleRate : taux d'échantillonnage actuel
    //  Explication : Nombre d'échantillons par seconde (ex: 44100 Hz)
    //    - Détermine la résolution temporelle de l'audio
    //    - Nécessaire pour calculer correctement les fréquences
    double currentSampleRate = 44100.0;

    // ================= Modules de traitement audio =================

    // adsr : générateur d'enveloppe ADSR (Attack, Decay, Sustain, Release)
    //    Module JUCE qui génère une courbe d'amplitude dans le temps
    //    Valeurs de 0.0 à 1.0, multipliées au signal audio
    juce::ADSR adsr;

    //  filterAdsr : enveloppe ADSR dédiée au filtre (NOUVEAU!)
    //  Explication : Contrôle la modulation de la cutoff dans le temps
    //    - Indépendante de l'ADSR d'amplitude
    //    - Permet des timbres dynamiques (ex: filter sweep)
    //    - Typique des synthés vintage (Moog, ARP, Roland)
    juce::ADSR filterAdsr;

    //  filter : filtre audio (State Variable TPT = Topology-Preserving Transform)
    //    Filtre numérique qui modifie le timbre en coupant certaines fréquences
    //    Contrôlé par cutoff (fréquence de coupure) et resonance (résonance)
    juce::dsp::StateVariableTPTFilter<float> filter;

    //  Paramètres du filtre stockés pour la modulation
    float baseCutoff = 1000.0f;      // Cutoff de base (sans modulation)
    float filterResonance = 0.7f;    // Résonance
    float filterEnvAmount = 0.0f;    // Intensité de la modulation (-100 à +100)

    //  Paramètres du générateur de bruit (NOUVEAU!)
    bool noiseEnabled = false;       // Activation du bruit
    float noiseLevel = 0.3f;         // Niveau du bruit (0.0-1.0)

    //  vintageProcessor : module de traitement vintage
    //  Explication : Ajoute le caractère analogique au son
    //    - Saturation douce (warmth, harmoniques)
    //    - Drift de pitch (instabilité subtile)
    //    - Bruit analogique (texture)
    //    → Transforme un son numérique froid en son vintage chaud
    VintageProcessor vintageProcessor;
};

