# soundtypes~

A Max/MSP external for real-time concatenative synthesis based on the soundtypes algorithm by Carmine E. Cella.

## What it does
- Analyses a corpus buffer~ using STFT spectral flux and MFCC features
- Segments and clusters the corpus into N soundtypes via KMeans
- Matches live microphone input to the nearest corpus segment in real time
- Outputs segment positions for playback via groove~ with crossfading

## Based on
- [soundtypes by Carmine E. Cella](https://github.com/CarmineCella/soundtypes)
- Paper: http://www.carminecella.com/research/icmc2013_cella_burred.pdf

## Build
Requires Max SDK and Xcode on macOS.

## Author
nicolacasetta
