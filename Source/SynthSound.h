/*
  ==============================================================================

    SynthSound.h
    Created: 30 Sep 2025 7:13:50am
    Author:  Ulysse Lebigre

    📌 RÔLE : Définit le "son" que le synthétiseur peut jouer.

    🎯 CONCEPT JUCE :
    - JUCE sépare "Sound" (quel type de son) et "Voice" (comment le générer)
    - Un Synthesiser peut avoir plusieurs sons différents (ex: piano, guitare...)
    - Chaque son décide s'il peut répondre à certaines notes/canaux MIDI

    💡 Dans notre cas : on crée un son "universel" qui répond à TOUT
    (toutes les notes, tous les canaux MIDI)

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// ================= Classe SynthSound =================
// Hérite de juce::SynthesiserSound (classe de base JUCE)
class SynthSound : public juce::SynthesiserSound
{
public:
    // ❓ Question : Cette note MIDI doit-elle déclencher ce son ?
    // 📝 Paramètre : midiNoteNumber (0-127, ex: 60 = Do central)
    // ✅ Réponse : true = oui, toujours (on accepte toutes les notes)
    // 💡 On pourrait filtrer ici (ex: return midiNoteNumber >= 60 pour notes aiguës uniquement)
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }

    // ❓ Question : Ce canal MIDI doit-il déclencher ce son ?
    // 📝 Paramètre : midiChannel (1-16)
    // ✅ Réponse : true = oui, toujours (on accepte tous les canaux)
    // 💡 Utile si on veut différents sons sur différents canaux (ex: batterie sur canal 10)
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};
