/*
  ==============================================================================

    Oscillator.h

    RÔLE : Générateur d'oscillateur multi-formes d'onde

    FORMES D'ONDE DISPONIBLES :
    - Sine     : onde sinusoïdale pure (son doux, pas d'harmoniques)
    - Saw      : dent de scie (riche en harmoniques, son brillant)
    - Square   : onde carrée (harmoniques impaires, son creux)
    - Triangle : onde triangulaire (harmoniques impaires atténuées, doux)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// ================= Énumération des formes d'onde =================
enum class OscillatorWaveform
{
    Sine,
    Saw,
    Square,
    Triangle
};

// ================= Classe Oscillator =================
class Oscillator
{
public:
    // Constructeur
    Oscillator() = default;

    // 🎚️ Définir la forme d'onde
    void setWaveform(OscillatorWaveform waveform)
    {
        currentWaveform = waveform;
    }

    // Définir la fréquence (en Hz)
    void setFrequency(double frequency, double sampleRate)
    {
        // Calculer l'incrément de phase par échantillon
        // Phase va de 0.0 à 1.0 (un cycle complet)
        auto cyclesPerSample = frequency / sampleRate;
        phaseDelta = cyclesPerSample;
    }

    // Générer le prochain échantillon
    // Explication : Génération avec anti-aliasing (PolyBLEP)
    //    - Sans anti-aliasing : son dur, numérique, aliasing à haute fréquence
    //    - Avec PolyBLEP : son doux, analogique, pas d'aliasing
    //    - Technique utilisée dans les synthés pros (Serum, Diva, etc.)
    float getNextSample()
    {
        float sample = 0.0f;

        // Générer le sample selon la forme d'onde sélectionnée
        switch (currentWaveform)
        {
            case OscillatorWaveform::Sine:
                // Onde sinusoïdale : sin(2π × phase)
                // Pas besoin d'anti-aliasing pour la sinusoïde (pas de discontinuités)
                sample = std::sin(currentPhase * 2.0 * juce::MathConstants<double>::pi);
                break;

            case OscillatorWaveform::Saw:
                // Dent de scie avec PolyBLEP anti-aliasing
                // Explication : La rampe brute crée de l'aliasing (son dur)
                //    - PolyBLEP adoucit les discontinuités → son vintage
                sample = (2.0f * currentPhase) - 1.0f;
                sample -= polyBlep(currentPhase, phaseDelta);  // Anti-aliasing magic!
                break;

            case OscillatorWaveform::Square:
                // Onde carrée avec PolyBLEP anti-aliasing
                // Explication : Le saut brutal de -1 à +1 crée de l'aliasing
                //    - PolyBLEP adoucit les deux transitions → son plus doux
                sample = currentPhase < 0.5 ? 1.0f : -1.0f;
                sample += polyBlep(currentPhase, phaseDelta);           // Transition à 0
                sample -= polyBlep(fmod(currentPhase + 0.5, 1.0), phaseDelta);  // Transition à 0.5
                break;

            case OscillatorWaveform::Triangle:
                // Onde triangulaire : rampe montante puis descendante
                // Pas besoin d'anti-aliasing (pas de discontinuités abruptes)
                if (currentPhase < 0.5)
                    sample = -1.0f + (4.0f * currentPhase);
                else
                    sample = 3.0f - (4.0f * currentPhase);
                break;
        }

        // Avancer la phase pour le prochain échantillon
        currentPhase += phaseDelta;

        // Garder la phase entre 0.0 et 1.0 (wrapping)
        if (currentPhase >= 1.0)
            currentPhase -= 1.0;

        return sample;
    }

    // 🔄 Réinitialiser la phase (pour éviter les clics au démarrage)
    void reset()
    {
        currentPhase = 0.0;
    }

    // PolyBLEP : Algorithme anti-aliasing (méthode privée)
    // Explication : PolyBLEP = Polynomial Bandlimited Step
    //    - Adoucit les discontinuités dans les formes d'onde
    //    - Simule le comportement analogique des vrais synthés vintage
    //    - Résultat : son plus chaud, moins numérique
    //    - Utilisé par : Serum, Diva, Phase Plant, etc.
    //
    // Paramètres :
    //    - t : position de phase actuelle (0.0 à 1.0)
    //    - dt : incrément de phase (vitesse)
    //
    // Fonctionnement :
    //    - Détecte les discontinuités (quand phase passe près de 0 ou 1)
    //    - Applique un polynôme pour adoucir la transition
    //    - Plus la fréquence est haute, plus l'adoucissement est important
    float polyBlep(double t, double dt)
    {
        // Discontinuité à t = 0 (début du cycle)
        if (t < dt)
        {
            t /= dt;
            // Polynôme parabolique : 2t² - 2t + 1/2
            return (float)(t + t - t * t - 1.0);
        }
        // Discontinuité à t = 1 (fin du cycle)
        else if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            // Polynôme parabolique inversé
            return (float)(t * t + t + t + 1.0);
        }
        // Pas de discontinuité → pas de correction
        return 0.0f;
    }

private:
    // ================= Variables privées =================
    OscillatorWaveform currentWaveform = OscillatorWaveform::Sine;
    double currentPhase = 0.0;  // Phase actuelle (0.0 à 1.0)
    double phaseDelta = 0.0;    // Incrément de phase par échantillon
};

