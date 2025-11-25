# 🚀 Guide d'optimisation DSP - Quick Wins

**Objectif** : Optimiser le moteur DSP de votre synthé avec les techniques de Helm
**Gain attendu** : 30-40% de performance
**Temps** : 30 min - 1h
**Difficulté** : ⭐⭐ Moyen

---

## 📊 Problèmes identifiés

### ❌ Problème 1 : Mise à jour excessive dans processBlock

**Fichier** : `PluginProcessor.cpp` lignes 334-346

**Code actuel** :
```cpp
// ❌ PROBLÈME : Met à jour TOUTES les voix à CHAQUE cycle
for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i))) {
        voice->updateADSR(adsrParams);          // Même si inchangé !
        voice->updateFilterADSR(filterAdsrParams);
        voice->updateFilter(...);
        voice->setWaveform(waveform);
        voice->updateUnison(voices, detune, stereo);
        voice->updateNoise(noiseEnable, noiseLevel);
    }
}
```

**Impact** :
- Appels inutiles si les paramètres n'ont pas changé
- Mise à jour des voix inactives (gaspillage CPU)
- Recalculs répétés à chaque bloc audio (~86 fois/sec à 44.1kHz)

---

## ✅ Solution 1 : Dirty Flags (Pattern Helm)

### Concept

Ne mettre à jour que :
1. ✅ Quand les paramètres ont **réellement changé**
2. ✅ Seulement les voix **actives**

### Étape 1 : Ajouter les flags dans PluginProcessor.h

**Ajoutez dans la section `private` (vers la fin du fichier)** :

```cpp
private:
    // ... (vos variables existantes)

    // ===== OPTIMISATION : Dirty Flags =====
    // 🎯 Évite les mises à jour inutiles des paramètres
    std::atomic<bool> adsrChanged{true};
    std::atomic<bool> filterAdsrChanged{true};
    std::atomic<bool> filterParamsChanged{true};
    std::atomic<bool> waveformChanged{true};
    std::atomic<bool> unisonChanged{true};
    std::atomic<bool> noiseChanged{true};

    // Cache des dernières valeurs
    juce::ADSR::Parameters lastAdsrParams;
    juce::ADSR::Parameters lastFilterAdsrParams;
    FilterParams lastFilterParams;
    OscillatorWaveform lastWaveform = OscillatorWaveform::Sine;
    int lastVoices = 1;
    float lastDetune = 0.0f;
    float lastStereo = 0.0f;
    bool lastNoiseEnable = false;
    float lastNoiseLevel = 0.0f;
};
```

### Étape 2 : Fonction helper pour comparer les paramètres

**Ajoutez dans PluginProcessor.h (section `private`)** :

```cpp
private:
    // Helper pour comparer les paramètres ADSR
    bool adsrParamsEqual(const juce::ADSR::Parameters& a,
                         const juce::ADSR::Parameters& b) const
    {
        return std::abs(a.attack - b.attack) < 0.001f &&
               std::abs(a.decay - b.decay) < 0.001f &&
               std::abs(a.sustain - b.sustain) < 0.001f &&
               std::abs(a.release - b.release) < 0.001f;
    }

    // Helper pour comparer les paramètres du filtre
    bool filterParamsEqual(const FilterParams& a, const FilterParams& b) const
    {
        return std::abs(a.cutoff - b.cutoff) < 0.1f &&
               std::abs(a.resonance - b.resonance) < 0.01f &&
               std::abs(a.envAmount - b.envAmount) < 0.01f;
    }
```

### Étape 3 : Modifier le processBlock

**Remplacez le code ligne 292-346 dans PluginProcessor.cpp par** :

```cpp
// 🎛️ ÉTAPE 3 : Récupérer et COMPARER les paramètres
auto adsrParams = getADSRParams();
auto filterParams = getFilterParams();
auto filterAdsrParams = getFilterADSRParams();
auto waveformIndex = parameters.getRawParameterValue("waveform")->load();
auto waveform = static_cast<OscillatorWaveform>(waveformIndex);
auto voices = (int)parameters.getRawParameterValue("voices")->load();
auto detune = parameters.getRawParameterValue("detune")->load() / 100.0f;
auto stereo = parameters.getRawParameterValue("stereo")->load() / 100.0f;
auto noiseEnable = parameters.getRawParameterValue("noiseEnable")->load() > 0.5f;
auto noiseLevel = parameters.getRawParameterValue("noiseLevel")->load();

// ✅ OPTIMISATION 1 : Détection des changements
// Ne pas mettre à jour si rien n'a changé !
if (!adsrParamsEqual(adsrParams, lastAdsrParams)) {
    lastAdsrParams = adsrParams;
    adsrChanged.store(true);
}

if (!adsrParamsEqual(filterAdsrParams, lastFilterAdsrParams)) {
    lastFilterAdsrParams = filterAdsrParams;
    filterAdsrChanged.store(true);
}

if (!filterParamsEqual(filterParams, lastFilterParams)) {
    lastFilterParams = filterParams;
    filterParamsChanged.store(true);
}

if (waveform != lastWaveform) {
    lastWaveform = waveform;
    waveformChanged.store(true);
}

if (voices != lastVoices ||
    std::abs(detune - lastDetune) > 0.001f ||
    std::abs(stereo - lastStereo) > 0.001f) {
    lastVoices = voices;
    lastDetune = detune;
    lastStereo = stereo;
    unisonChanged.store(true);
}

if (noiseEnable != lastNoiseEnable ||
    std::abs(noiseLevel - lastNoiseLevel) > 0.001f) {
    lastNoiseEnable = noiseEnable;
    lastNoiseLevel = noiseLevel;
    noiseChanged.store(true);
}

// ✅ OPTIMISATION 2 : Mise à jour conditionnelle et voix actives uniquement
for (int i = 0; i < synth.getNumVoices(); ++i) {
    if (auto* voice = dynamic_cast<SynthVoice*>(synth.getVoice(i))) {
        // ✅ Vérifier si la voix est active
        if (!voice->isVoiceActive())
            continue;  // Skip les voix inactives !

        // ✅ Mettre à jour seulement ce qui a changé
        if (adsrChanged.load())
            voice->updateADSR(adsrParams);

        if (filterAdsrChanged.load())
            voice->updateFilterADSR(filterAdsrParams);

        if (filterParamsChanged.load())
            voice->updateFilter(filterParams.cutoff,
                              filterParams.resonance,
                              filterParams.envAmount);

        if (waveformChanged.load())
            voice->setWaveform(waveform);

        if (unisonChanged.load())
            voice->updateUnison(voices, detune, stereo);

        if (noiseChanged.load())
            voice->updateNoise(noiseEnable, noiseLevel);
    }
}

// ✅ Réinitialiser les flags après mise à jour
adsrChanged.store(false);
filterAdsrChanged.store(false);
filterParamsChanged.store(false);
waveformChanged.store(false);
unisonChanged.store(false);
noiseChanged.store(false);
```

### Étape 4 : Ajouter isVoiceActive() dans SynthVoice.h

**Ajoutez dans la section `public` de SynthVoice.h** :

```cpp
public:
    // ... (vos méthodes existantes)

    // ✅ OPTIMISATION : Vérifier si la voix est active
    // Évite de mettre à jour les voix silencieuses
    bool isVoiceActive() const override
    {
        return isVoiceActive();  // Méthode héritée de SynthesiserVoice
    }
```

---

## ✅ Solution 2 : Éviter les recalculs dans getADSRParams()

### Problème

Les fonctions `getADSRParams()`, `getFilterADSRParams()`, etc. sont appelées **à chaque bloc** même si rien n'a changé.

### Solution : Cacher les valeurs

**Modifiez les fonctions dans PluginProcessor.h** :

```cpp
// Cache pour éviter les lectures répétées
mutable juce::ADSR::Parameters cachedAdsrParams;
mutable juce::ADSR::Parameters cachedFilterAdsrParams;
mutable FilterParams cachedFilterParams;
mutable bool adsrCacheValid = false;
mutable bool filterAdsrCacheValid = false;
mutable bool filterCacheValid = false;

juce::ADSR::Parameters getADSRParams() const
{
    // ✅ Utiliser le cache si valide
    if (adsrCacheValid)
        return cachedAdsrParams;

    cachedAdsrParams.attack  = *parameters.getRawParameterValue("attack");
    cachedAdsrParams.decay   = *parameters.getRawParameterValue("decay");
    cachedAdsrParams.sustain = *parameters.getRawParameterValue("sustain");
    cachedAdsrParams.release = *parameters.getRawParameterValue("release");
    adsrCacheValid = true;

    return cachedAdsrParams;
}

// Faire de même pour getFilterADSRParams() et getFilterParams()
```

**Invalidez le cache quand les paramètres changent** :

Ajoutez un Listener dans PluginProcessor.cpp (constructeur) :

```cpp
// Dans le constructeur, après la création des paramètres
parameters.addParameterListener("attack", this);
parameters.addParameterListener("decay", this);
parameters.addParameterListener("sustain", this);
parameters.addParameterListener("release", this);
// ... (tous les autres paramètres)
```

Et implémentez le listener :

```cpp
// Dans PluginProcessor.h, héritez de AudioProcessorValueTreeState::Listener
class SYNTH_1AudioProcessor : public juce::AudioProcessor,
                               public juce::AudioProcessorValueTreeState::Listener
{
    // ...

    // Implémentation du listener
    void parameterChanged(const juce::String& parameterID, float newValue) override
    {
        // Invalider les caches appropriés
        if (parameterID == "attack" || parameterID == "decay" ||
            parameterID == "sustain" || parameterID == "release")
            adsrCacheValid = false;

        if (parameterID == "filterAttack" || parameterID == "filterDecay" ||
            parameterID == "filterSustain" || parameterID == "filterRelease")
            filterAdsrCacheValid = false;

        if (parameterID == "cutoff" || parameterID == "resonance" ||
            parameterID == "filterEnvAmount")
            filterCacheValid = false;
    }
};
```

---

## 📊 Résultats attendus

### Avant optimisation
- 8 voix × 6 updates = **48 appels/bloc**
- Même si rien n'a changé
- Même pour les voix inactives

### Après optimisation
- 0 appels si aucun paramètre n'a changé
- Seulement les voix actives (ex: 2-3 voix)
- 2-3 voix × updates nécessaires seulement

### Gain estimé : 30-40% de CPU

---

## 🧪 Test des performances

### Avant de commencer
```bash
# Dans votre DAW (Logic Pro)
# Ouvrir le moniteur CPU
# Créer une piste avec 8 notes simultanées
# Noter le % de CPU utilisé
```

### Après chaque optimisation
```bash
# Recompiler
./rebuild.sh  # (si vous avez le script)
# Ou recompiler dans Xcode

# Tester dans Logic Pro
# Comparer le % de CPU
```

---

## ✅ Checklist

- [ ] Ajouté les dirty flags dans PluginProcessor.h
- [ ] Ajouté les fonctions de comparaison
- [ ] Modifié le processBlock avec détection de changements
- [ ] Ajouté isVoiceActive() dans SynthVoice.h
- [ ] Implémenté le système de cache (optionnel)
- [ ] Compilé sans erreurs
- [ ] Testé dans Logic Pro
- [ ] Mesuré le gain de performance

---

## 🚀 Prochaines étapes (après Quick Wins)

Une fois ces optimisations en place, vous pourrez :
- [ ] Ajouter des lookup tables pour les waveforms (gain +15-25%)
- [ ] Utiliser SIMD avec juce::dsp (gain +40-60%)
- [ ] Implémenter l'architecture modulaire de Helm (maintenabilité)

---

## 💡 Conseils

1. **Testez étape par étape**
   - Faites un commit git avant de commencer
   - Appliquez une optimisation à la fois
   - Vérifiez que ça compile après chaque étape

2. **Mesurez vraiment**
   - Notez le % CPU avant
   - Notez le % CPU après
   - Le gain peut varier selon votre machine

3. **Debugging**
   - Si ça ne compile pas, vérifiez les includes
   - Si le son est bizarre, vérifiez les flags
   - Les atomics doivent être en `private`

---

**Bon courage ! N'hésitez pas si vous avez des questions. 🎹**
