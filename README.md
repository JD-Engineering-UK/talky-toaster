# Talky-toaster

Inspired by the television show Red Dwarf.

This repository contains the firmware for a real-life version of the talky-toaster using a mmWave presence sensor, an audio amplifier and an ESP32S3 microcontroller.

## Features
- OTA update
- Time aware (NTP sync over WiFi)
- MP3 decompression and playback
- Motion sensor reading (including distance)

## TODO:
- [ ] Telemetry
- [ ] Configuration
- [ ] Silent OTA Update (Boot sounds play after OTA update but I don't want sounds playing out of hours)

## Possible Future Development:
- [ ] LED lights when the toaster is "speaking"
- [ ] Integrate more closely with a toaster. Possibly to detect how well-done the toast is or perhaps to simply have a voice line when the toaster lever is pressed down.
- [ ] Implement "wake word detection" to detect rejections of the toasters offers of grilled bread products and respond accordingly