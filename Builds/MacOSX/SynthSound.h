//
//  SynthSound.h
//  SYNTH_1
//
//  Created by Ulysse Lebigre on 30/09/2025.
//

#pragma once
#include <JuceHeader.h>

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote(int /*midiNoteNumber*/) override { return true; }
    bool appliesToChannel(int /*midiChannel*/) override { return true; }
};
