/*
  ==============================================================================

    SpectrumAnalyzer.h

    📌 RÔLE : Analyseur de spectre en temps réel (comme Serum, Vital)

    🎨 AFFICHAGE :
    - Spectre de fréquences animé
    - Visualisation temps réel du son
    - Dégradé de couleurs (bas = bleu, haut = rouge)

    🔧 TECHNIQUE :
    - FFT (Fast Fourier Transform) pour analyser les fréquences
    - Buffer circulaire pour stocker l'audio
    - Rendu à 30-60 FPS pour fluidité

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <atomic>

class SpectrumAnalyzer : public juce::Component,
                         private juce::Timer
{
public:
    // 🏗️ Constructeur
    // 📝 Explication : Initialise l'analyseur FFT
    //    - FFT Order 11 = 2048 points (bon compromis précision/performance)
    //    - Plus l'order est élevé, plus c'est précis mais lent
    SpectrumAnalyzer()
        : forwardFFT(fftOrder),
          window(fftSize, juce::dsp::WindowingFunction<float>::hann)
    {
        // Initialiser tous les buffers à zéro
        fifo.fill(0.0f);
        fftData.fill(0.0f);
        scopeData.fill(0.0f);

        // Démarrer le timer de rafraîchissement (30 FPS)
        // 📝 Explication : L'analyseur se redessine 30 fois par seconde
        //    - 30 FPS = fluidité suffisante sans surcharger le CPU
        //    - 60 FPS serait mieux mais consomme 2x plus
        startTimerHz(30);
    }

    // 🎵 Envoyer de l'audio à analyser (thread-safe)
    void pushNextSampleIntoFifo(float sample) noexcept
    {
        // Ajouter le sample au FIFO
        int index = fifoIndex.load();

        if (index < fftSize)
        {
            fifo[index] = sample;
            fifoIndex.store(index + 1);
        }

        // Si le FIFO est plein, on calcule la FFT
        if (index >= fftSize - 1)
        {
            // Ne calculer que si les données précédentes ont été affichées
            bool expected = false;
            if (nextFFTBlockReady.compare_exchange_strong(expected, true))
            {
                // 🔄 Copier les données dans le buffer FFT
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());

                // 🪟 Appliquer une fenêtre (Hann window)
                window.multiplyWithWindowingTable(fftData.data(), fftSize);

                // ⚡ Effectuer la FFT
                forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

                // 📊 Calculer les magnitudes
                for (int i = 0; i < scopeSize; ++i)
                {
                    auto skewedProportionX = 1.0f - std::exp(std::log(1.0f - (float)i / (float)scopeSize) * 0.2f);
                    auto fftDataIndex = juce::jlimit(0, fftSize / 2, (int)(skewedProportionX * fftSize * 0.5f));

                    // Conversion en dB plus sûre
                    auto rawValue = fftData[fftDataIndex];
                    auto level = juce::jlimit(0.0f, 1.0f, rawValue * 2.0f);

                    scopeData[i] = level;
                }
            }

            // 🔄 Réinitialiser le FIFO
            fifoIndex.store(0);
        }
    }

    // 🎨 Dessiner l'analyseur
    // 📝 Explication : Rendu visuel du spectre
    //    - Appelé automatiquement par JUCE
    //    - Dessine les barres de fréquences avec dégradés
    void paint(juce::Graphics& g) override
    {
        // 🌑 Fond vintage sombre (comme un oscilloscope vintage)
        g.fillAll(juce::Colour(0xff1a1a1a));

        // 📏 Dimensions de la zone d'affichage
        auto width = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();

        // 🎨 Si on a des données, on dessine le spectre
        if (nextFFTBlockReady)
        {
            // 🎨 Dessiner chaque bin de fréquence
            auto binWidth = width / (float)scopeSize;

            for (int i = 0; i < scopeSize; ++i)
            {
                // 📊 Hauteur de la barre (en fonction de la magnitude)
                auto barHeight = scopeData[i] * height * 0.9f;

                // 🧡 Couleur VINTAGE : Orange chaud qui s'intensifie
                auto normalizedHeight = scopeData[i];
                juce::Colour barColour = juce::Colour(0xffff8c42).withAlpha(0.3f + normalizedHeight * 0.7f);

                // Éclat au sommet (comme un VU-mètre vintage)
                if (normalizedHeight > 0.7f)
                    barColour = juce::Colour(0xffd4af37); // Doré brillant

                // 🎨 Dessiner la barre style VU-mètre
                g.setColour(barColour);
                g.fillRect(i * binWidth, height - barHeight, binWidth - 1, barHeight);
            }

            // 📏 Grille vintage (comme un oscilloscope)
            g.setColour(juce::Colour(0xffff8c42).withAlpha(0.15f));
            for (int i = 1; i < 4; ++i)
            {
                auto y = height * i / 4;
                g.drawLine(0, (float)y, (float)width, (float)y, 1.0f);
            }

            // Grille verticale
            for (int i = 1; i < 8; ++i)
            {
                auto x = width * i / 8;
                g.drawLine((float)x, 0, (float)x, (float)height, 1.0f);
            }

            // ✅ Marquer les données comme affichées
            nextFFTBlockReady.store(false);
        }

        // 🔲 Bordure dorée vintage
        g.setColour(juce::Colour(0xffd4af37).withAlpha(0.6f));
        g.drawRect(getLocalBounds(), 2);
    }

private:
    // ⏱️ Timer callback : rafraîchir l'affichage
    // 📝 Explication : Appelé 30 fois par seconde
    //    - Force le redessin (repaint)
    //    - Crée l'animation fluide
    void timerCallback() override
    {
        repaint();
    }

    // ================= Paramètres FFT =================

    static constexpr int fftOrder = 11;              // Order 11 = 2048 points
    static constexpr int fftSize = 1 << fftOrder;    // 2048
    static constexpr int scopeSize = 512;            // Nombre de bins à afficher

    // ================= Objets FFT =================

    juce::dsp::FFT forwardFFT;                       // Moteur FFT de JUCE
    juce::dsp::WindowingFunction<float> window;      // Fenêtre de Hann

    // ================= Buffers de données =================

    std::array<float, fftSize> fifo;                 // FIFO circulaire pour l'audio entrant
    std::array<float, fftSize * 2> fftData;          // Buffer FFT (besoin de 2x la taille)
    std::array<float, scopeSize> scopeData;          // Données à afficher (magnitudes)

    std::atomic<int> fifoIndex { 0 };                // Position dans le FIFO (atomic pour thread-safety)
    std::atomic<bool> nextFFTBlockReady { false };   // Nouvelles données disponibles ? (atomic)
};
