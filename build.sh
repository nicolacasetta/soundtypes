#!/bin/bash
set -e

echo ">>> copying source to SDK..."
cp ~/Documents/_code/soundtypes/source/soundtypes.c ~/Documents/Max\ 9/max-sdk/source/projects/soundtypes/
cp ~/Documents/_code/soundtypes/source/CMakeLists.txt ~/Documents/Max\ 9/max-sdk/source/projects/soundtypes/

echo ">>> clean build..."
cd ~/Documents/Max\ 9/max-sdk/build
rm -rf source/projects/soundtypes/CMakeFiles
rm -rf ~/Documents/Max\ 9/max-sdk/externals/soundtypes.mxo
cmake --build . --target soundtypes

echo ">>> installing..."
rm -rf ~/Documents/Max\ 9/Packages/soundtypes/externals/soundtypes~.mxo
cp -r ~/Documents/Max\ 9/max-sdk/externals/soundtypes.mxo ~/Documents/Max\ 9/Packages/soundtypes/externals/soundtypes~.mxo

echo ">>> signing..."
codesign --force --deep --sign - ~/Documents/Max\ 9/Packages/soundtypes/externals/soundtypes~.mxo

echo ">>> done — restart Max to load new version"

echo ">>> updating repo binary..."
cp -r ~/Documents/Max\ 9/Packages/soundtypes/externals/soundtypes~.mxo ~/Documents/_code/soundtypes/externals/macOS/soundtypes~.mxo
echo ">>> repo binary updated — commit and push when ready"
