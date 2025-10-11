/*
  ==============================================================================

    VintageProcessor.h

    📌 RÔLE : Module de traitement "vintage" pour simuler l'analogique

    🎛️ FONCTIONNALITÉS :
    - Saturation douce (warmth/harmoniques)
    - Drift analogique (instabilité de pitch)
    - Bruit analogique subtil

    🎨 POURQUOI ?
    - Les synthés numériques sonnent "trop propres"
    - Les vrais synthés analogiques ont des imperfections
    - Ces imperfections créent le caractère "chaud" et "vivant"

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class VintageProcessor
{
public:
    VintageProcessor() = default;

    // 🔥 Saturation douce (type tube/transistor)
    // 📝 Explication : Ajoute des harmoniques chaleureuses
    //    - Simule la saturation naturelle des circuits analogiques
    //    - Fonction tanh (tangente hyperbolique) = saturation douce
    //    - Plus l'entrée est forte, plus la saturation est audible
    //    - Résultat : son plus chaud, plus "gras", plus analogique
    //
    // Utilisé dans : Moog, ARP, Oberheim, etc.
    float softClip(float sample)
    {
        // Amount de saturation (1.0 à 3.0)
        // 📝 Plus le gain est élevé, plus la saturation est marquée
        //    - 1.0 = transparent (presque pas de saturation)
        //    - 1.5 = saturation subtile (recommandé)
        //    - 3.0 = saturation marquée (type distorsion douce)
        const float gain = 1.5f;

        // Appliquer la saturation tanh
        // 📝 tanh(x) compresse progressivement le signal vers ±1
        //    - Entrée faible → sortie linéaire (pas de changement)
        //    - Entrée forte → sortie compressée (saturation)
        //    - Ajoute des harmoniques impaires (2e, 3e, 5e...)
        sample = std::tanh(sample * gain);

        // Compenser le gain pour garder un niveau cohérent
        return sample * 0.8f;
    }

    // 🌊 Générateur de drift analogique (instabilité de pitch)
    // 📝 Explication : Les oscillateurs analogiques dérivent légèrement
    //    - Température, composants, alimentation → pitch instable
    //    - Crée un son "vivant" vs numérique "figé"
    //    - LFO ultra-lent (0.1-0.5 Hz) avec bruit brownien
    //    - Amplitude très faible (± 0.5 à 2 cents max)
    //
    // Utilisé dans : Minimoog, Juno-60, Prophet-5
    float getDriftAmount(double sampleRate)
    {
        // 🎲 Génération de bruit brownien (random walk)
        // 📝 Explication : Le pitch ne saute pas, il dérive progressivement
        //    - Chaque sample = petit pas aléatoire
        //    - Crée une courbe fluide et organique
        //    - Plus réaliste qu'un LFO pur
        static juce::Random random;
        driftPhase += (random.nextFloat() - 0.5f) * 0.0001f;  // Petit pas aléatoire

        // 📏 Limiter la dérive pour rester subtil
        // 📝 Trop de drift = désaccordé / Trop peu = inutile
        //    ± 0.0005 = environ ±1 cent (imperceptible mais perceptible)
        if (driftPhase > 0.0005f) driftPhase = 0.0005f;
        if (driftPhase < -0.0005f) driftPhase = -0.0005f;

        return driftPhase;
    }

    // 🔊 Ajouter du bruit analogique subtil (MODIFIÉ!)
    // 📝 Explication : Les circuits analogiques génèrent du bruit thermique
    //    - Bruit blanc contrôlable par l'utilisateur
    //    - Ajoute de la "texture" au son
    //    - Niveau contrôlé par l'onglet NOISE
    //
    // Utilisé dans : Tous les synthés analogiques vintage
    float addAnalogNoise(bool enabled, float level)
    {
        if (!enabled)
            return 0.0f;  // Pas de bruit si désactivé

        static juce::Random random;
        // Bruit blanc avec niveau ajustable
        // level = 0.0 à 1.0 (converti depuis 0-100%)
        // Base de 0.0003f (bruit très subtil) multiplié par le niveau
        return (random.nextFloat() * 2.0f - 1.0f) * 0.0003f * level * 100.0f;
    }

private:
    // État du drift (position actuelle de la dérive)
    float driftPhase = 0.0f;
};
