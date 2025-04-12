## 1.4 relation freq & interval
- natural glissandi over notes, `mtof~`, freq perception not linear
- 0db is max, db are negative numbers, 6db drop = half loudness
  - -6 db = amplitude of 0.5, `dbtoa`
- `line~`, `curve~` -> linear, expo ramp generators
  - can be used to interoplate values with `append`
- `sfplay~`, `sfrecord`, `playlist` -> read and write audio from disk
- `buffer~`, `groove~` read and write audio from memory
- `toggle` sends 1 or 0

## 1.6 panning
- signal power varies in proportion to square of amplitude
  - to preserve loudness use sqrt of amp mult factor

## 1.7 Max basics
- objects that end with ~ are MSP
- MAX messages are passed around slow as packages of data
- MSP signals are continously generated, at the sample rate
 - `what then is the difference between gen~ and msp`
- MAX objects execute right to left

## 2.1 Fixed spectrum additive synthesis
- component has amplitude, frequency, phase
- components of a complex sound are `harmonics` if they are `integer multiples`
  of some lowest component `fundamental`
  - these are also called `overtones`
- non integer multiples are `partials`
- the angle of sin() is the phase of the waveform
- waves in `antiphase` get eliminated due to `destructive interference`
- if a sound has no information on the fundamental the pitch doesnt change
  and most likely we still can identefy the note
- relationships between non harmonics are irrational
  - `non periodic waveform`, no pitch preceived
- sounds can contain both non and harmonic components
- 1hz fundamental cannot be identified by ear as tune -> not hearable, to far
- periodic sounds can be preceived as non-harmonic

- `harmonic sound` has integer multiple spaced components of fundamental
- brain can reconstruct fundamental to a ceratain degree
- if brain cant reconstruct, periodic sound is preceived as non-harmonic
- fundamental of periodic sound is `greatest common divisor`
- non-periodic sound does not repeat the cycle
- non-periodic can have defined pitch and get preceived as harmonic
  - `quasi harmonic sounds`
  - that is if the components are close to integer multiples
- non-periodic `fixed spectrum` cant be reproduced on a computer -> irrational
  - but the period can be nearly infinitely long
- `variable spectrum` sounds can easily be non-periodic

- `fixed spectrum additive synthesis` can be done in two ways
  - summing sine oscilators
  - summing sine wave components to a complex wavetable/waveform
    and then send to one oscillator

## 2.2 Beats
- multiple signals that go in and out of phase
- frequency is f2-f1, preceived freq is (f1+f2)/2
- the `critical band` is the max phase difference still preceived as beats
  where the ear cannot seperate two notes
- above 200hz critical band with increases
  `low frequencies only wide intervals are preceived as consonant`
- more shared harmonics -> more consonant
  - whole and semi tone are inside critical band -> dissonant
- complex sounds multiple beats

## 2.3 Crossfading
- wavetables can be faded from one into the other -> vector synthesis

## Variable spectrum additive synthesis
- other word for timbre, is a multidimensional quantity
  expresses the amplitude, freq, phase for every component of a sound
- most practical method for complex spectrum sound is
  `analysis of real sound and then re-synthesis` with FFT

## 2P
- build a additive synth that eats cpu performance, then
  render the wave into a wavetable?
  - what happends if feed a short buffer (low res samples) into an osc (wave~)?
- read the gen~ chapter 2, implementing also simple wt oscillator
- build `https://www.youtube.com/watch?v=wSh0rXyJOPo&t=189s` additive osc gen~
- build `https://www.youtube.com/watch?v=2432oaAcCyg` interpolation wt

- `wt vs additive` wt is static, even for inharmonic components
  additive wave does move for inharmonic
  - so to save inarmonic additive synthesis to wt one needs to snapshot
    the resulting wave so that it can be srcolled trough to recreate the
    movement of the additive inharmonic oscillation
  - a snapshot of inharmonic additive signal does still contain inharmonic info
    it just doesnt move







## TIL
- negative `groove~` reverse is musicaly interresting
- tuning, notation and keyboard layout is interesting and diverse
- multiple oscillators that are slightly out of phase -> interesting beats



## Shortcuts
- cmd in edit mode to use stuff like in performance mode
