/*
  ==============================================================================

    UnisonOscillator.h

     RÔLE : Oscillateur avec Unison (plusieurs voix désaccordées)

     CONCEPT UNISON :
    - Au lieu de 1 oscillateur → on en utilise plusieurs (3-7 typiquement)
    - Chaque oscillateur est légèrement désaccordé (detune)
    - Quand on les mixe → son ÉPAIS, RICHE, STÉRÉO
    - Technique utilisée dans : Roland Juno, SuperSaw, Serum

     EXEMPLE SONORE :
    - 1 voix : son fin, mono
    - 3 voix + detune : son plus riche
    - 7 voix + detune : son massif type SuperSaw

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "Oscillator.h"

class UnisonOscillator
{
public:
    //  Constructeur
    //  Explication : Initialise les oscillateurs en unison
    //    - Par défaut : 1 voix (pas d'unison)
    //    - Maximum : 7 voix (SuperSaw style)
    UnisonOscillator()
    {
        // Créer 7 oscillateurs (on n'utilisera que numVoices)
        for (int i = 0; i < maxVoices; ++i)
            oscillators.add(new Oscillator());
    }

    //  Définir le nombre de voix unison (1-7)
    //  Explication : Plus de voix = son plus épais
    //    - 1 voix : son normal (pas d'unison)
    //    - 3 voix : unison subtil, riche
    //    - 5 voix : unison marqué, large
    //    - 7 voix : SuperSaw massif (Trance, EDM)
    void setNumVoices(int num)
    {
        numVoices = juce::jlimit(1, maxVoices, num);
    }

    //  Définir la quantité de détune (0.0 à 1.0)
    //  Explication : Contrôle le désaccordage entre les voix
    //    - 0.0 = pas de détune (toutes les voix à l'unisson parfait)
    //    - 0.5 = détune subtil (recommandé, ±5 cents)
    //    - 1.0 = détune fort (±15 cents, très chorus)
    void setDetuneAmount(float amount)
    {
        detuneAmount = juce::jlimit(0.0f, 1.0f, amount);
    }

    //  Définir la largeur stéréo (0.0 à 1.0)
    //  Explication : Répartit les voix dans l'espace stéréo
    //    - 0.0 = toutes les voix au centre (mono)
    //    - 0.5 = répartition modérée (recommandé)
    //    - 1.0 = répartition maximale (extrême gauche/droite)
    void setStereoWidth(float width)
    {
        stereoWidth = juce::jlimit(0.0f, 1.0f, width);
    }

    //  Définir la forme d'onde pour tous les oscillateurs
    void setWaveform(OscillatorWaveform waveform)
    {
        for (auto* osc : oscillators)
            osc->setWaveform(waveform);
    }

    //  Définir la fréquence
    //  Explication : Configure tous les oscillateurs avec détune
    //    - La voix centrale reste à la fréquence exacte
    //    - Les autres voix sont désaccordées symétriquement
    void setFrequency(double frequency, double sampleRate)
    {
        for (int i = 0; i < numVoices; ++i)
        {
            // Calculer le détune pour cette voix
            // 📝 Formule : détune symétrique autour de la voix centrale
            //    - Voix 0 (centre) : détune = 0
            //    - Voix 1, 2 : détune positif
            //    - Voix -1, -2 : détune négatif
            float voiceDetune = 0.0f;

            if (numVoices > 1)
            {
                // Index centré : -1, 0, +1 pour 3 voix
                int voiceIndex = i - (numVoices / 2);

                // Détune en cents (1 cent = 1/100 de demi-ton)
                //  Formule : voiceIndex × detuneAmount × maxDetuneCents
                //    - maxDetuneCents = 15 cents (standard Unison)
                //    - Exemple avec 3 voix, detune=0.5 :
                //      → Voix 0 : -7.5 cents
                //      → Voix 1 : 0 cents
                //      → Voix 2 : +7.5 cents
                const float maxDetuneCents = 15.0f;
                float detuneCents = voiceIndex * detuneAmount * maxDetuneCents;

                // Convertir cents en ratio de fréquence
                //  Formule : 2^(cents/1200)
                //    - 100 cents = 1 demi-ton = 2^(1/12)
                //    - 15 cents ≈ 1.00865 (0.865% plus haut)
                voiceDetune = std::pow(2.0f, detuneCents / 1200.0f);
            }

            // Appliquer le détune à la fréquence
            oscillators[i]->setFrequency(frequency * voiceDetune, sampleRate);
        }
    }

    //  Générer le prochain échantillon STÉRÉO
    //  Explication : Mix toutes les voix avec panoramique stéréo
    //    - Retourne un std::pair<float, float> = (gauche, droite)
    //    - Les voix sont réparties dans l'espace stéréo
    std::pair<float, float> getNextSampleStereo()
    {
        float leftSum = 0.0f;
        float rightSum = 0.0f;

        for (int i = 0; i < numVoices; ++i)
        {
            // Générer le sample de cette voix
            float sample = oscillators[i]->getNextSample();

            // Calculer le panoramique pour cette voix
            //  Formule : répartition linéaire de gauche à droite
            //    - Voix 0 → pan = -1.0 (gauche)
            //    - Voix centrale → pan = 0.0 (centre)
            //    - Dernière voix → pan = +1.0 (droite)
            float pan = 0.0f;
            if (numVoices > 1)
            {
                // Pan de -1.0 (gauche) à +1.0 (droite)
                pan = (2.0f * i / (numVoices - 1)) - 1.0f;
                pan *= stereoWidth;  // Appliquer la largeur stéréo
            }

            // Convertir le pan en gains gauche/droite
            //  Formule : loi du pan constant power
            //    - pan = -1 → left=1.0, right=0.0 (tout à gauche)
            //    - pan = 0 → left=0.707, right=0.707 (centre, -3dB)
            //    - pan = +1 → left=0.0, right=1.0 (tout à droite)
            float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
            float leftGain = std::cos(panAngle);
            float rightGain = std::sin(panAngle);

            // Ajouter ce sample aux canaux gauche/droite
            leftSum += sample * leftGain;
            rightSum += sample * rightGain;
        }

        // Normaliser par le nombre de voix (éviter la saturation)
        //  Explication : Plus de voix → plus d'accumulation
        //    - Division par sqrt(numVoices) pour garder le volume perçu
        //    - Formule : volume perçu ∝ √nombre_de_sources
        float normFactor = 1.0f / std::sqrt((float)numVoices);

        return {leftSum * normFactor, rightSum * normFactor};
    }

    //  Réinitialiser toutes les phases
    void reset()
    {
        for (auto* osc : oscillators)
            osc->reset();
    }

private:
    static constexpr int maxVoices = 7;  // Maximum 7 voix (SuperSaw standard)

    juce::OwnedArray<Oscillator> oscillators;  // Liste des oscillateurs
    int numVoices = 1;                         // Nombre de voix actives
    float detuneAmount = 0.5f;                 // Quantité de détune (0-1)
    float stereoWidth = 0.5f;                  // Largeur stéréo (0-1)
};

