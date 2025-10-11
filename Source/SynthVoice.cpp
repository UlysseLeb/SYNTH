/*
  ==============================================================================

    SynthVoice.cpp

    📌 IMPLÉMENTATION de la classe SynthVoice
    Contient toute la logique de génération du son

  ==============================================================================
*/

#include "SynthVoice.h"

// ================= Vérification de compatibilité =================
// ❓ Question : Cette voix peut-elle jouer ce son ?
// 🔍 Utilise dynamic_cast pour vérifier que le son est bien un SynthSound
// ✅ Retourne true si le cast réussit (= c'est bien notre type de son)
bool SynthVoice::canPlaySound(juce::SynthesiserSound* sound)
{
    return dynamic_cast<SynthSound*>(sound) != nullptr;
}

// ================= DÉMARRAGE D'UNE NOTE =================
// 🎹 Appelé quand une touche MIDI est enfoncée (Note On)
// ⚡ C'est ici qu'on initialise tous les paramètres pour jouer la note
void SynthVoice::startNote(int midiNoteNumber, float velocity,
                           juce::SynthesiserSound* /*sound*/,
                           int /*currentPitchWheelPosition*/)
{
    // 🎼 ÉTAPE 1 : Convertir la note MIDI en fréquence (Hz)
    // 📝 Explication : Les notes MIDI sont des numéros (0-127)
    //    - Note 60 = Do central (261.6 Hz)
    //    - Note 69 = La (440 Hz) - référence de l'accordage
    //    - Formule : fréquence = 440 * 2^((note - 69) / 12)
    //    - Chaque demi-ton = multiplication par 2^(1/12) ≈ 1.0595
    currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

    // 📊 ÉTAPE 2 : Récupérer le sample rate
    // 📝 Explication : Le sample rate définit la qualité audio
    //    - 44100 Hz = CD quality (44100 échantillons par seconde)
    //    - 48000 Hz = standard professionnel
    //    - 96000 Hz = haute définition
    currentSampleRate = getSampleRate();

    // 🎵 ÉTAPE 3 : Configurer l'oscillateur avec la fréquence
    // 📝 Explication : L'oscillateur a besoin de connaître :
    //    - La fréquence de la note (en Hz)
    //    - Le sample rate (pour calculer l'incrément de phase correct)
    oscillator.setFrequency(currentFrequency, currentSampleRate);

    // 🔊 ÉTAPE 4 : Définir l'amplitude de base
    // 📝 Explication : La vélocité MIDI représente la force de frappe
    //    - MIDI velocity = 0 à 127 (converti en 0.0 à 1.0 par JUCE)
    //    - On multiplie par 0.15 pour laisser de la marge (headroom)
    //    - Sans headroom, plusieurs notes simultanées → saturation/distorsion
    level = velocity * 0.15;

    // 🔧 ÉTAPE 5 : Réinitialiser la phase de l'oscillateur
    // 📝 Explication : Évite les clics au démarrage
    //    - Si on ne reset pas, l'oscillateur reprend où il s'était arrêté
    //    - Cela crée une discontinuité brutale = CLIC audible
    //    - reset() remet la phase à 0 = démarrage propre
    oscillator.reset();

    // 📈 ÉTAPE 6 : Démarrer les enveloppes ADSR
    // 📝 Explication : Lance la phase "Attack" pour les deux enveloppes
    //    - ADSR amplitude : contrôle le volume
    //    - ADSR filtre : contrôle la cutoff (timbre)
    //    - Les deux sont synchronisées au démarrage de la note
    adsr.noteOn();
    filterAdsr.noteOn();  // NOUVEAU : démarrer l'enveloppe du filtre
}







// ================= ARRÊT D'UNE NOTE =================
// 🛑 Appelé quand une touche MIDI est relâchée (Note Off)
// 🎯 Objectif : arrêter la note proprement (avec ou sans "queue")
void SynthVoice::stopNote(float /*velocity*/, bool allowTailOff)
{
    // 📉 ÉTAPE 1 : Signaler aux enveloppes ADSR que la note est relâchée
    // 📝 Explication : Lance la phase "Release" pour les deux enveloppes
    //    - ADSR amplitude : descente du volume
    //    - ADSR filtre : fermeture du filtre (si env amount > 0)
    //    - Crée un arrêt naturel et musical
    adsr.noteOff();
    filterAdsr.noteOff();  // NOUVEAU : arrêter l'enveloppe du filtre

    // ❓ ÉTAPE 2 : Doit-on arrêter immédiatement ou laisser la release se terminer ?
    // 📝 Explication : Deux cas possibles :
    //    - allowTailOff = false → PANIC mode (Ctrl+All Notes Off dans le DAW)
    //      → Arrêt immédiat sans attendre la release
    //    - ADSR déjà inactive → la release est terminée naturellement
    //      → On peut libérer la voix pour qu'elle joue une autre note
    if (!allowTailOff || !adsr.isActive())
    {
        // 🧹 Nettoyage : libère cette voix pour qu'elle puisse jouer une autre note
        // 📝 Explication : Le synthétiseur a un nombre limité de voix (8)
        //    - clearCurrentNote() indique "cette voix est libre"
        //    - La prochaine note MIDI pourra utiliser cette voix
        //    - Sans ça, on tomberait rapidement à court de voix disponibles
        clearCurrentNote();
    }
}

// ================= GÉNÉRATION AUDIO (CŒUR DU SYNTHÉTISEUR) =================
// 🔊 Appelé en boucle pour remplir le buffer audio avec le son généré
// ⚡ C'EST ICI QUE LE SON EST CRÉÉ, SAMPLE PAR SAMPLE !
// 📝 Explication du flux audio :
//    Note MIDI → Oscillateur → ADSR → Filtre → Buffer de sortie → Haut-parleurs
void SynthVoice::renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    // 🚦 Vérification : cette voix est-elle active ?
    // 📝 Explication : Si l'ADSR est inactive, aucune note n'est jouée
    //    - Pas besoin de générer de l'audio (économise du CPU)
    //    - isActive() = true pendant Attack, Decay, Sustain, Release
    //    - isActive() = false quand la Release est terminée
    if (!adsr.isActive())
        return;  // Sortie anticipée : pas d'audio à générer

    // 🔄 BOUCLE PRINCIPALE : génère chaque échantillon audio
    // 📝 Explication : Le traitement audio se fait sample par sample
    //    - À 44100 Hz, on génère 44100 samples par seconde
    //    - Chaque sample = une valeur entre -1.0 et +1.0
    //    - Le buffer typique = 512 samples (≈ 11ms à 44.1 kHz)
    while (--numSamples >= 0)
    {
        // 📊 ÉTAPE 1 : Obtenir les valeurs des enveloppes ADSR
        // 📝 Explication : Deux enveloppes indépendantes
        //    - envValue : contrôle l'amplitude (volume)
        //    - filterEnvValue : contrôle la cutoff du filtre (timbre)
        auto envValue = adsr.getNextSample();
        auto filterEnvValue = filterAdsr.getNextSample();  // NOUVEAU : enveloppe du filtre

        // 🎚️ ÉTAPE 1.5 : Moduler la fréquence de coupure du filtre avec l'enveloppe
        // 📝 Explication : Filter sweep dynamique (typique des synthés vintage)
        //    - baseCutoff : fréquence de base (réglée par l'utilisateur)
        //    - filterEnvValue : enveloppe 0.0 à 1.0 (monte pendant attack)
        //    - filterEnvAmount : intensité de la modulation (-100 à +100)
        //    - Si positif : filtre s'ouvre (cutoff augmente)
        //    - Si négatif : filtre se ferme (cutoff diminue)

        // Calculer la modulation en fonction de l'enveloppe
        // filterEnvAmount positif -> ajoute jusqu'à +5000Hz à la cutoff
        // filterEnvAmount négatif -> retire jusqu'à -5000Hz de la cutoff
        float modulation = filterEnvValue * (filterEnvAmount / 100.0f) * 5000.0f;
        auto modulatedCutoff = baseCutoff + modulation;

        // Limiter entre 20Hz et 20kHz pour rester audible
        modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);
        filter.setCutoffFrequency(modulatedCutoff);

        // 🌊 ÉTAPE 2 : Générer l'échantillon audio STÉRÉO depuis l'oscillateur Unison
        // 📝 Explication : L'oscillateur génère maintenant un son stéréo !
        //    - getNextSampleStereo() retourne {gauche, droite}
        //    - Plusieurs voix désaccordées mixées en stéréo
        //    - Son beaucoup plus riche et large qu'un oscillateur simple
        auto [rawLeft, rawRight] = oscillator.getNextSampleStereo();

        // 🔊 ÉTAPE 3 : Appliquer l'amplitude (vélocité MIDI × ADSR)
        // 📝 Explication : Combine deux sources de volume
        //    - level : volume de base (force de frappe MIDI)
        //    - envValue : modulation temporelle (enveloppe)
        //    - Résultat : un son qui monte/descend naturellement
        auto ampLeft = rawLeft * level * envValue;
        auto ampRight = rawRight * level * envValue;

        // 🎚️ ÉTAPE 4 : Appliquer le filtre aux deux canaux (avec cutoff modulée!)
        // 📝 Explication : Le filtre modifie le timbre (couleur du son)
        //    - processSample(0, left) → canal gauche
        //    - processSample(1, right) → canal droit
        //    - Filtre low-pass : coupe les hautes fréquences
        //    - La cutoff évolue grâce à l'enveloppe (son dynamique!)
        auto filteredLeft = filter.processSample(0, ampLeft);
        auto filteredRight = filter.processSample(1, ampRight);

        // 🎨 ÉTAPE 5 : Traitement vintage sur les deux canaux
        // 📝 Explication : Ajoute le caractère analogique au son
        //    - Saturation douce : harmoniques chaleureuses
        //    - Bruit analogique contrôlable : texture ajustable
        //    - Le bruit suit l'enveloppe d'amplitude (envValue)
        //    → Transforme un son numérique en son vintage
        filteredLeft = vintageProcessor.softClip(filteredLeft);
        filteredRight = vintageProcessor.softClip(filteredRight);

        // Ajouter le bruit avec l'enveloppe d'amplitude pour qu'il suive le volume
        float noise = vintageProcessor.addAnalogNoise(noiseEnabled, noiseLevel);
        filteredLeft += noise * envValue;   // Le bruit suit l'enveloppe AMP
        filteredRight += noise * envValue;  // Le bruit suit l'enveloppe AMP

        // 🔊 ÉTAPE 6 : Ajouter les samples traités aux canaux gauche et droite
        // 📝 Explication : Vraie sortie stéréo maintenant !
        //    - Canal 0 = gauche (filteredLeft)
        //    - Canal 1 = droite (filteredRight)
        //    - addSample() += accumulation (permet la polyphonie)
        //    - Plusieurs voix s'ajoutent dans le même buffer
        if (outputBuffer.getNumChannels() > 0)
            outputBuffer.addSample(0, startSample, filteredLeft);
        if (outputBuffer.getNumChannels() > 1)
            outputBuffer.addSample(1, startSample, filteredRight);

        // ⏭️ ÉTAPE 7 : Passer au prochain sample dans le buffer
        // 📝 Explication : On remplit le buffer séquentiellement
        //    - startSample commence souvent à 0
        //    - On l'incrémente jusqu'à remplir tout le buffer
        ++startSample;
    }

    // 🧹 NETTOYAGE : vérifier si la note est terminée
    // 📝 Explication : Libérer la voix quand la release est finie
    //    - isActive() = false → release terminée
    //    - clearCurrentNote() rend la voix disponible
    //    - Permet la polyphonie : une autre note peut utiliser cette voix
    if (!adsr.isActive())
    {
        clearCurrentNote();
    }
}






// ================= MISE À JOUR DU FILTRE =================
// 🎚️ Appelé depuis le processeur pour ajuster le filtre en temps réel
// 📝 Configure la fréquence de coupure, la résonance et l'intensité de l'enveloppe
void SynthVoice::updateFilter(float cutoff, float resonance, float envAmount)
{
    // 🎚️ ÉTAPE 1 : Stocker les paramètres du filtre
    // 📝 Explication : Ces valeurs sont stockées et utilisées dans renderNextBlock()
    //    - baseCutoff : fréquence de base (sans modulation)
    //    - filterResonance : résonance du filtre
    //    - filterEnvAmount : intensité de la modulation par l'enveloppe
    baseCutoff = cutoff;
    filterResonance = resonance;
    filterEnvAmount = envAmount;

    // 🔊 ÉTAPE 2 : Définir la résonance (Q factor)
    // Amplifie les fréquences autour de la coupure
    // Bas = filtre doux, Haut = son nasillard/métallique
    filter.setResonance(resonance);

    // 📝 Note : La cutoff est maintenant modulée en temps réel dans renderNextBlock()
    //    On ne la définit plus ici de manière statique
}

