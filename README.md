# soundtypes~

A Max/MSP external for real-time concatenative synthesis based on the soundtypes algorithm by Carmine E. Cella (https://github.com/CarmineCella).

Analyse a corpus of sound, then use a live microphone to navigate it in real time. The external finds the corpus segment whose timbre most closely matches what it hears, and outputs its position for playback.

## How it works

1. Load a sound file into a buffer~
2. Send bang to analyse it — the external segments it using spectral flux, extracts MFCC features, and clusters segments with KMeans
3. Connect a live audio source (mic, instrument) to the signal inlet
4. The external continuously matches the live audio to the nearest corpus segment and outputs its sample position
5. Use groove~ to play back the matched segment in a loop

## Installation

### macOS (pre-compiled)

1. Copy externals/macOS/soundtypes~.mxo to your Max packages folder:
   ~/Documents/Max 9/Packages/soundtypes/externals/

2. Remove the quarantine flag:
   xattr -cr ~/Documents/Max\ 9/Packages/soundtypes/externals/soundtypes~.mxo

3. Restart Max

### Build from source

Requires Xcode and the Max SDK.

   git clone https://github.com/nicolacasetta/soundtypes
   cd soundtypes
   ./build.sh

## Messages

| Message          | Description                                                        |
|------------------|--------------------------------------------------------------------|
| bang             | Analyse the corpus buffer — must send after set                    |
| set name         | Set the corpus buffer~ name                                        |
| clusters n       | Number of KMeans clusters (default: 3)                             |
| sensitivity 0-1  | Peak picking threshold — lower = more segments (default: 0.25)     |
| minlength ms     | Minimum segment length in milliseconds (default: 200)              |
| threshold f      | Mic energy gate — below this RMS value matching stops (default: 0.01) |

## Outlets

| Outlet | Output                                                      |
|--------|-------------------------------------------------------------|
| Left   | segment start_sample end_sample — matched segment position  |
| Right  | match index cluster distance — match diagnostics            |

## Typical patch

   [adc~ 1]          [buffer~ corpus]
       |                    |
       |               [read]   <- click to load sound file
       |
   [soundtypes~]
       |         |
       |         +-- match info (optional)
   [route segment]
       |
   [unpack 0 0]
     |       |
   / 44.1  / 44.1    <- samples to milliseconds
     |       |
   2nd     3rd inlet of [groove~ corpus]
                 |
             [ezdac~]

## Parameters guide

How many segments?
Start with sensitivity 0.25 and minlength 200. The console tells you how many segments
were found after each bang. For speech/voice material try sensitivity 0.15.
For dense music try sensitivity 0.4.

Mic triggers in silence?
Increase threshold — try values between 0.02 and 0.05.

Segments too short?
Increase minlength — try 500 or 1000 ms.

All segments sound the same?
Increase clusters and use a longer, more varied corpus file.

## Based on

- soundtypes by Carmine E. Cella: https://github.com/CarmineCella/soundtypes
- Cella, C.E. and Burred, J.J. — "Soundtypes: a system for automatic segmentation,
  classification and composition with audio corpora", ICMC 2013

## Author

Nicola Casetta
