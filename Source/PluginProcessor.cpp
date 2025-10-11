/*
  ==============================================================================

    PluginProcessor.cpp

    📌 IMPLÉMENTATION du processeur audio principal
    Contient toute la logique de traitement MIDI et audio

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SynthVoice.h"
#include "SynthSound.h"
#include "Oscillator.h"  // 📦 Nécessaire pour OscillatorWaveform enum

// ================= Constructeur =================
// 🏗️ Appelé à la création du plugin (chargement dans le DAW)
SYNTH_1AudioProcessor::SYNTH_1AudioProcessor()
    // 🔌 Initialisation de la classe de base AudioProcessor
    // Configure les bus audio (entrées/sorties)
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        // Si ce n'est PAS un synthé → ajouter une entrée stéréo
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        // Tous les plugins ont une sortie stéréo
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    // 🎛️ Initialisation de l'arbre de paramètres
    // Paramètres :
    //   - *this : référence au processeur
    //   - nullptr : pas d'undo manager
    //   - "SYNTH_1Params" : identifiant unique
    //   - createParameterLayout() : structure des paramètres (attack, decay, etc.)
    parameters(*this, nullptr, juce::Identifier("SYNTH_1Params"), createParameterLayout())
{
    // ❌ Corps vide : évite un bug avec le format Audio Unit (macOS)
    // L'initialisation des voix se fait dans prepareToPlay() à la place
}

// ================= Destructeur =================
// 🧹 Appelé à la destruction du plugin (fermeture du DAW)
// Rien à nettoyer manuellement : JUCE gère tout automatiquement
SYNTH_1AudioProcessor::~SYNTH_1AudioProcessor() {}

// ================= Création des paramètres =================
// 🏗️ Définit TOUS les paramètres contrôlables du synthétiseur
// Appelé dans le constructeur pour initialiser l'arbre de paramètres
juce::AudioProcessorValueTreeState::ParameterLayout SYNTH_1AudioProcessor::createParameterLayout()
{
    // 📦 Vecteur pour stocker tous les paramètres
    // std::unique_ptr = pointeur intelligent (gestion mémoire automatique)
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // ================= Paramètres ADSR (Enveloppe) =================

    // 📈 ATTACK : temps de montée (0.01s à 5s, défaut 0.1s)
    // Contrôle la rapidité d'apparition du son après Note On
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"attack", 1},  // ID : "attack", version 1
        "Attack",                          // Nom affiché
        0.01f, 5.0f,                       // Min, Max
        0.1f));                            // Valeur par défaut

    // 📉 DECAY : temps de descente (0.01s à 5s, défaut 0.1s)
    // Contrôle le temps pour passer du pic au niveau Sustain
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"decay", 1}, "Decay", 0.01f, 5.0f, 0.1f));

    // 🔊 SUSTAIN : niveau de maintien (0 à 1, défaut 0.8)
    // Niveau du son tant que la touche reste enfoncée
    // 0.0 = silence, 1.0 = amplitude maximale
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"sustain", 1}, "Sustain", 0.0f, 1.0f, 0.8f));

    // 📉 RELEASE : temps d'extinction (0.01s à 5s, défaut 0.1s)
    // Contrôle le temps de fade-out après Note Off
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID {"release", 1}, "Release", 0.01f, 5.0f, 0.1f));

    // ================= Paramètres du filtre =================

    // 🎚️ CUTOFF : fréquence de coupure (20 Hz à 20 kHz, défaut 1000 Hz)
    // 📝 Explication : Contrôle la brillance du son
    //    - 20-500 Hz : son très sombre, étouffé
    //    - 1000-5000 Hz : son équilibré, naturel
    //    - 10000-20000 Hz : son très brillant, aéré
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"cutoff", 1}, "Cutoff", 20.0f, 20000.0f, 1000.0f));

    // 🔊 RESONANCE : résonance du filtre (0.1 à 10, défaut 1.0)
    // 📝 Explication : Boost autour de la fréquence de coupure
    //    - 0.1-1.0 : filtre doux, naturel
    //    - 2.0-5.0 : caractère marqué, synthétique
    //    - 7.0-10.0 : son nasillard, métallique, presque auto-oscillant
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"resonance", 1}, "Resonance", 0.1f, 10.0f, 1.0f));

    // ================= ADSR du filtre (NOUVEAU!) =================
    // 📝 Explication : Enveloppe séparée pour moduler le filtre dans le temps
    //    - Permet d'ouvrir/fermer le filtre indépendamment du volume
    //    - Classique sur les synthés pros (Moog, Prophet, Juno)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterAttack", 1}, "Filter Attack", 0.01f, 5.0f, 0.3f));  // Défaut 0.3s (plus audible)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterDecay", 1}, "Filter Decay", 0.01f, 5.0f, 0.8f));  // Défaut 0.8s

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterSustain", 1}, "Filter Sustain", 0.0f, 1.0f, 0.3f));  // Défaut 0.3 (30%)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterRelease", 1}, "Filter Release", 0.01f, 5.0f, 0.5f));  // Défaut 0.5s

    // 🎚️ FILTER ENV AMOUNT : intensité de la modulation du filtre (-100% à +100%, défaut +80%)
    // 📝 Explication : Contrôle l'impact de l'enveloppe sur la fréquence de coupure
    //    - 0% = pas de modulation (cutoff statique)
    //    - +100% = modulation maximale vers les aigus (+5000Hz)
    //    - -100% = modulation inverse (ferme le filtre, -5000Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"filterEnvAmount", 1}, "Filter Env Amount", -100.0f, 100.0f, 80.0f));  // Défaut 80% pour effet audible

    // ================= Paramètres NOISE (NOUVEAU!) =================
    // 📝 Explication : Générateur de bruit blanc pour enrichir le son
    //    - Typique des synthés vintage (ajout de "souffle" et texture)
    //    - Suit l'enveloppe d'amplitude pour une intégration naturelle

    // 🔊 NOISE ENABLE : activer/désactiver le générateur de bruit (défaut OFF)
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"noiseEnable", 1}, "Noise Enable", false));

    // 🎚️ NOISE LEVEL : niveau du bruit (0-100%, défaut 30%)
    // 📝 Volume du bruit mixé avec l'oscillateur principal
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"noiseLevel", 1}, "Noise Level", 0.0f, 100.0f, 30.0f));

    // ================= Paramètres de l'oscillateur =================

    // 🎵 WAVEFORM : forme d'onde de l'oscillateur (0-3, défaut 0 = Sine)
    // 📝 Explication : Détermine le timbre de base du son
    //    - 0 = Sine : son pur, doux, pas d'harmoniques (flûte, pad)
    //    - 1 = Saw : son brillant, riche en harmoniques (cordes, lead)
    //    - 2 = Square : son creux, vintage (8-bit, clarinette)
    //    - 3 = Triangle : son doux, quelques harmoniques (flûte, pad doux)
    // AudioParameterChoice = menu déroulant avec des choix textuels
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"waveform", 1},
        "Waveform",
        juce::StringArray{"Sine", "Saw", "Square", "Triangle"},  // Options du menu
        0));  // Index par défaut (0 = Sine)

    // ================= Paramètres Unison =================

    // 🎵 VOICES : nombre de voix unison (1-7)
    // 📝 Explication : Plus de voix = son plus épais
    //    - 1 = pas d'unison (son normal)
    //    - 3 = unison subtil (recommandé)
    //    - 7 = SuperSaw massif (Trance/EDM)
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{"voices", 1},
        "Voices",
        1, 7,    // Min, Max
        3));     // Défaut : 3 voix

    // 🎚️ DETUNE : quantité de désaccordage (0-100%)
    // 📝 Explication : Contrôle l'écart de fréquence entre les voix
    //    - 0% = toutes les voix parfaitement accordées (inutile)
    //    - 50% = détune subtil (±7.5 cents, naturel)
    //    - 100% = détune fort (±15 cents, chorus marqué)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"detune", 1},
        "Detune",
        0.0f, 100.0f,  // Min, Max (en pourcentage)
        50.0f));       // Défaut : 50%

    // 🎧 STEREO : largeur stéréo de l'unison (0-100%)
    // 📝 Explication : Répartit les voix dans l'espace stéréo
    //    - 0% = toutes les voix au centre (mono)
    //    - 50% = répartition modérée (recommandé)
    //    - 100% = répartition maximale (extrême L/R)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stereo", 1},
        "Stereo",
        0.0f, 100.0f,  // Min, Max (en pourcentage)
        50.0f));       // Défaut : 50%

    // 📤 Retourne la structure complète des paramètres
    // { params.begin(), params.end() } = tous les éléments du vecteur
    return { params.begin(), params.end() };
}


// ================= Préparation avant traitement audio =================
// 🎬 Appelé quand le DAW démarre ou change de configuration audio
// ⚡ C'est ici qu'on initialise le synthétiseur !
void SYNTH_1AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // 🎵 ÉTAPE 1 : Configurer le sample rate du synthétiseur
    // Indispensable pour que les voix calculent correctement leurs fréquences
    // Ex: 44100 Hz, 48000 Hz, 96000 Hz...
    synth.setCurrentPlaybackSampleRate(sampleRate);

    // 🗑️ ÉTAPE 2 : Nettoyer les anciennes voix (si reload)
    synth.clearVoices();

    // 🎹 ÉTAPE 3 : Créer 8 voix pour la polyphonie
    // Chaque voix peut jouer une note indépendamment
    // → Le synthé peut jouer 8 notes simultanément maximum
    for (int i = 0; i < 8; ++i)
        synth.addVoice(new SynthVoice());

    // 🗑️ ÉTAPE 4 : Nettoyer les anciens sons (si reload)
    synth.clearSounds();

    // 🔊 ÉTAPE 5 : Ajouter notre son (SynthSound)
    // Le synthétiseur a maintenant 8 voix qui peuvent jouer ce son
    synth.addSound(new SynthSound());

    // 🎛️ ÉTAPE 6 : Préparer chaque voix (initialiser filtre et ADSR)
    // Nécessaire pour que les modules DSP fonctionnent correctement
    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            // Prépare le filtre et l'ADSR avec les paramètres audio actuels
            voice->prepareVoice(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
        }
    }

    // ✅ VÉRIFICATIONS DE SÉCURITÉ
    // jassert = comme un "assert" mais version JUCE
    // Crash en mode Debug si les conditions ne sont pas remplies
    jassert(synth.getNumVoices() > 0);  // Au moins 1 voix
    jassert(synth.getNumSounds() > 0);  // Au moins 1 son
}

// ================= Libération des ressources =================
// 🧹 Appelé quand le DAW arrête ou met en pause
// Rien à faire ici : JUCE nettoie automatiquement
void SYNTH_1AudioProcessor::releaseResources() {}

// ================= Configuration des bus audio =================
// 🔌 Vérifie si le plugin supporte une configuration audio donnée
// Le DAW appelle cette méthode pour savoir quelles configurations sont OK
#ifndef JucePlugin_PreferredChannelConfigurations
bool SYNTH_1AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsSynth
    // 🎹 MODE SYNTHÉTISEUR : pas d'entrée audio, seulement sortie stéréo
    // On accepte uniquement une sortie stéréo (gauche + droite)
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
#else
    // 🎛️ MODE EFFET AUDIO : entrée ET sortie stéréo requises
    // Ex: VST sur une piste audio dans Ableton
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
#endif
}
#endif

// ================= TRAITEMENT AUDIO ET MIDI (CŒUR DU PLUGIN) =================
// ⚡ Appelé en boucle par le DAW pour générer l'audio
// 🔥 FONCTION TEMPS-RÉEL : doit être ULTRA rapide (pas d'allocation mémoire !)
// Ex: à 44100 Hz avec buffer de 512 samples → appelé ~86 fois par seconde
void SYNTH_1AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer& midiMessages)
{
    // 🛡️ Protection contre les "denormals" (nombres très petits)
    // Les denormals ralentissent le CPU → on les force à 0
    // ScopedNoDenormals = actif seulement dans cette fonction
    juce::ScopedNoDenormals noDenormals;

    // 🧹 ÉTAPE 1 : Vider le buffer audio
    // Important pour un synthé (pas d'entrée audio à traiter)
    // Met tous les samples à 0.0
    buffer.clear();

    // 🎹 ÉTAPE 2 : Mettre à jour l'état du clavier MIDI virtuel
    // Synchronise le clavier affiché dans l'interface avec les notes jouées
    // Paramètres :
    //   - midiMessages : messages MIDI entrants
    //   - 0 : position de départ dans le buffer
    //   - buffer.getNumSamples() : nombre de samples à traiter
    //   - true : injecter les événements du clavier virtuel
    keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), true);

    // 🎛️ ÉTAPE 3 : Récupérer les paramètres ADSR actuels
    // Les sliders peuvent changer en temps réel → on récupère les valeurs à jour
    auto adsrParams = getADSRParams();

    // 🎚️ ÉTAPE 3.5 : Récupérer les paramètres du filtre
    // 📝 Explication : Les sliders de filtre peuvent changer en temps réel
    //    - On lit les valeurs actuelles depuis l'arbre de paramètres
    //    - Ces valeurs sont automatiquement synchronisées avec l'interface
    auto filterParams = getFilterParams();

    // 🎵 ÉTAPE 3.6 : Récupérer la forme d'onde sélectionnée
    // 📝 Explication : Le menu déroulant retourne un index (0-3)
    //    - 0 = Sine, 1 = Saw, 2 = Square, 3 = Triangle
    //    - On cast l'index en OscillatorWaveform (enum)
    auto waveformIndex = parameters.getRawParameterValue("waveform")->load();
    auto waveform = static_cast<OscillatorWaveform>(waveformIndex);

    // 🎚️ ÉTAPE 3.7 : Récupérer les paramètres Unison
    // 📝 Explication : L'unison crée un son épais avec plusieurs voix
    //    - voices : nombre de voix (1-7)
    //    - detune : écart de fréquence (0-100%)
    //    - stereo : largeur stéréo (0-100%)
    auto voices = (int)parameters.getRawParameterValue("voices")->load();
    auto detune = parameters.getRawParameterValue("detune")->load() / 100.0f;  // Convertir % en 0-1
    auto stereo = parameters.getRawParameterValue("stereo")->load() / 100.0f;  // Convertir % en 0-1

    // 🔊 ÉTAPE 3.8 : Récupérer les paramètres NOISE (NOUVEAU!)
    // 📝 Explication : Le bruit blanc enrichit le son avec de la texture
    //    - noiseEnable : true/false (activer/désactiver)
    //    - noiseLevel : 0-100% (niveau du bruit)
    auto noiseEnable = parameters.getRawParameterValue("noiseEnable")->load() > 0.5f;
    auto noiseLevel = parameters.getRawParameterValue("noiseLevel")->load();

    // 🔄 ÉTAPE 4 : Mettre à jour TOUS les paramètres de TOUTES les voix
    // 📝 Explication : On parcourt les 8 voix du synthétiseur
    //    - Chaque voix doit être synchronisée avec les paramètres actuels
    //    - Permet de changer l'ADSR, le filtre, la forme d'onde en temps réel
    //    - Dynamic_cast vérifie que c'est bien une SynthVoice (sécurité)

    // 📊 Récupérer les paramètres ADSR du filtre (NOUVEAU!)
    auto filterAdsrParams = getFilterADSRParams();

    for (int i = 0; i < synth.getNumVoices(); ++i)
    {
        // Cast vers SynthVoice* pour accéder à nos méthodes custom
        if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i)))
        {
            voice->updateADSR(adsrParams);  // Met à jour l'enveloppe d'amplitude
            voice->updateFilterADSR(filterAdsrParams);  // Met à jour l'enveloppe du filtre
            voice->updateFilter(filterParams.cutoff, filterParams.resonance, filterParams.envAmount);  // Met à jour le filtre avec envAmount
            voice->setWaveform(waveform);  // Met à jour la forme d'onde
            voice->updateUnison(voices, detune, stereo);  // Met à jour l'unison
            voice->updateNoise(noiseEnable, noiseLevel);  // Met à jour le bruit (NOUVEAU!)
        }
    }

    // 🎵 ÉTAPE 5 : GÉNÉRER L'AUDIO !
    // Le synthétiseur :
    //   1. Lit les messages MIDI (note on/off)
    //   2. Distribue les notes aux voix disponibles
    //   3. Chaque voix génère son audio (renderNextBlock)
    //   4. Mixe toutes les voix dans le buffer
    // Paramètres :
    //   - buffer : buffer audio à remplir
    //   - midiMessages : événements MIDI à traiter
    //   - 0 : position de départ dans le buffer
    //   - buffer.getNumSamples() : nombre de samples à générer
    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // 🎚️ ÉTAPE 5.5 : Compensation perceptuelle Fletcher-Munson (PROFESSIONNEL!)
    // 📝 Explication : L'oreille humaine ne perçoit pas toutes les fréquences de la même façon
    //    - Les graves (50-200Hz) sont perçues beaucoup plus fort
    //    - Les aigus (3-8kHz) nécessitent un boost pour être perçus au même niveau
    //    - Cette courbe compense pour un rendu équilibré comme sur les synthés pros
    //    - Similaire à la courbe A-weighting mais optimisée pour les synthés
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);

        // Filtres IIR pour compensation perceptuelle
        static float hp_x1[2] = {0}, hp_x2[2] = {0}, hp_y1[2] = {0}, hp_y2[2] = {0};  // Passe-haut 40Hz
        static float shelf_x1[2] = {0}, shelf_x2[2] = {0}, shelf_y1[2] = {0}, shelf_y2[2] = {0};  // Shelf 200Hz
        static float peak_x1[2] = {0}, peak_x2[2] = {0}, peak_y1[2] = {0}, peak_y2[2] = {0};  // Peak 4kHz

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = channelData[i];

            // 1️⃣ Passe-haut à 40Hz (enlève les sub-bass inutiles et DC)
            float hp_a0 = 1.0f, hp_a1 = -1.98223f, hp_a2 = 0.98229f;
            float hp_b0 = 0.99114f, hp_b1 = -1.98223f, hp_b2 = 0.99114f;

            float hp_out = hp_b0 * sample + hp_b1 * hp_x1[channel] + hp_b2 * hp_x2[channel]
                         - hp_a1 * hp_y1[channel] - hp_a2 * hp_y2[channel];

            hp_x2[channel] = hp_x1[channel];
            hp_x1[channel] = sample;
            hp_y2[channel] = hp_y1[channel];
            hp_y1[channel] = hp_out;

            // 2️⃣ Low-shelf à 200Hz : -3dB (atténuation douce des graves)
            float shelf_a0 = 1.0f, shelf_a1 = -1.93477f, shelf_a2 = 0.93772f;
            float shelf_b0 = 0.97067f, shelf_b1 = -1.93477f, shelf_b2 = 0.96704f;

            float shelf_out = shelf_b0 * hp_out + shelf_b1 * shelf_x1[channel] + shelf_b2 * shelf_x2[channel]
                            - shelf_a1 * shelf_y1[channel] - shelf_a2 * shelf_y2[channel];

            shelf_x2[channel] = shelf_x1[channel];
            shelf_x1[channel] = hp_out;
            shelf_y2[channel] = shelf_y1[channel];
            shelf_y1[channel] = shelf_out;

            // 3️⃣ Peak à 4kHz : +2dB (boost subtil des aigus pour clarté)
            float peak_a0 = 1.0f, peak_a1 = -1.74453f, peak_a2 = 0.76863f;
            float peak_b0 = 1.03159f, peak_b1 = -1.74453f, peak_b2 = 0.73704f;

            float peak_out = peak_b0 * shelf_out + peak_b1 * peak_x1[channel] + peak_b2 * peak_x2[channel]
                           - peak_a1 * peak_y1[channel] - peak_a2 * peak_y2[channel];

            peak_x2[channel] = peak_x1[channel];
            peak_x1[channel] = shelf_out;
            peak_y2[channel] = peak_y1[channel];
            peak_y1[channel] = peak_out;

            channelData[i] = peak_out * 0.92f;  // Gain de compensation réduit
        }
    }

    // 📊 ÉTAPE 6 : Alimenter l'analyseur de spectre (NOUVEAU!)
    // 📝 Explication : Envoyer les samples audio à l'analyseur pour visualisation
    //    - On prend le canal gauche (0) pour l'analyse
    //    - L'analyseur effectuera la FFT pour afficher le spectre
    if (auto* editor = dynamic_cast<SYNTH_1AudioProcessorEditor*>(getActiveEditor()))
    {
        auto& analyzer = editor->getSpectrumAnalyzer();
        auto* channelData = buffer.getReadPointer(0);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            analyzer.pushNextSampleIntoFifo(channelData[i]);
        }
    }
}




// ================= Création de l'interface graphique =================
// 🖥️ Appelé quand l'utilisateur ouvre la fenêtre du plugin dans le DAW
juce::AudioProcessorEditor* SYNTH_1AudioProcessor::createEditor()
{
    // Crée une nouvelle instance de notre éditeur (interface graphique)
    // *this = référence au processeur (pour accéder aux paramètres)
    return new SYNTH_1AudioProcessorEditor(*this);
}

// ================= Factory function (point d'entrée du plugin) =================
// 🏭 Appelé par le DAW pour créer une instance du plugin
// JUCE_CALLTYPE = convention d'appel spécifique à la plateforme
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SYNTH_1AudioProcessor();
}

// ================= Informations sur le plugin =================
// Toutes ces méthodes sont appelées par le DAW pour connaître les capacités du plugin

// ❓ Le plugin a-t-il une interface graphique ?
bool SYNTH_1AudioProcessor::hasEditor() const { return true; }

// 📝 Nom du plugin (défini dans le .jucer)
const juce::String SYNTH_1AudioProcessor::getName() const { return JucePlugin_Name; }

// 🎹 Le plugin accepte-t-il le MIDI en entrée ? → OUI (c'est un synthé)
bool SYNTH_1AudioProcessor::acceptsMidi() const { return true; }

// 🎵 Le plugin produit-il du MIDI en sortie ? → NON (il produit de l'audio)
bool SYNTH_1AudioProcessor::producesMidi() const { return false; }

// 🎛️ Le plugin est-il un effet MIDI pur (sans audio) ? → NON
bool SYNTH_1AudioProcessor::isMidiEffect() const { return false; }

// ⏱️ Durée de la "queue" après arrêt (ex: reverb qui continue)
// Notre synthé s'arrête immédiatement après la release → 0.0 secondes
double SYNTH_1AudioProcessor::getTailLengthSeconds() const { return 0.0; }

// ================= Gestion des presets (non implémenté) =================
// Ces fonctions sont obligatoires mais on ne les utilise pas pour l'instant

// 📦 Nombre de presets disponibles → 1 (preset par défaut)
int SYNTH_1AudioProcessor::getNumPrograms() { return 1; }

// 🎯 Index du preset actuel → toujours 0
int SYNTH_1AudioProcessor::getCurrentProgram() { return 0; }

// 🔄 Charger un preset → ne fait rien
void SYNTH_1AudioProcessor::setCurrentProgram(int) {}

// 📝 Nom d'un preset → retourne une chaîne vide
const juce::String SYNTH_1AudioProcessor::getProgramName(int) { return {}; }

// ✏️ Renommer un preset → ne fait rien
void SYNTH_1AudioProcessor::changeProgramName(int, const juce::String&) {}

// ================= Sauvegarde / Chargement de l'état =================

// 💾 SAUVEGARDE : sauvegarder l'état du plugin dans un fichier
// Appelé quand :
//   - L'utilisateur sauvegarde son projet DAW
//   - Le plugin est désactivé/fermé
//   - Le DAW fait un snapshot pour l'undo
void SYNTH_1AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // 📝 Crée un flux de sortie vers le bloc mémoire
    // true = ajouter les données (append mode)
    juce::MemoryOutputStream stream(destData, true);

    // 💾 Écrit TOUS les paramètres dans le flux
    // parameters.state = arbre contenant attack, decay, sustain, release, cutoff, resonance
    // JUCE sérialise automatiquement toutes les valeurs en XML/binaire
    parameters.state.writeToStream(stream);
}

// 📂 RESTAURATION : charger l'état du plugin depuis un fichier
// Appelé quand :
//   - L'utilisateur ouvre un projet DAW sauvegardé
//   - Le plugin est activé/ouvert
//   - L'utilisateur fait un undo
void SYNTH_1AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // 📖 Lit les données binaires et reconstruit l'arbre de paramètres
    // ValueTree = structure de données JUCE (comme un JSON/XML)
    juce::ValueTree tree = juce::ValueTree::readFromData(data, sizeInBytes);

    // ✅ Vérifier que les données sont valides
    // Peut échouer si le fichier est corrompu ou d'une ancienne version
    if (tree.isValid())
        // 🔄 Restaure tous les paramètres
        // Les sliders de l'interface se mettront à jour automatiquement !
        parameters.state = tree;
}

