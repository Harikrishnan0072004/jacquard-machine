#!/bin/bash
echo "Compiling ESP32 project with Huge App partition scheme..."
arduino-cli compile --fqbn esp32:esp32:esp32:PartitionScheme=huge_app --output-dir ./build .

if [ $? -ne 0 ]; then
    echo "Compilation FAILED!"
    exit 1
fi

echo "Compilation SUCCESSFUL!"
cp -r ./build /mnt/c/Users/ishar/Desktop/esp32_build
echo "build folder is copy to /mnt/c/Users/ishar/Desktop/esp32_build "
