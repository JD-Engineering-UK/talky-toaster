# Talky-toaster

Inspired by the television show Red Dwarf.

This repository contains the firmware for a real-life version of the talky-toaster using a mmWave presence sensor, an audio amplifier and an ESP32S3 microcontroller.

## TODO:
- [x] Read distance data from mmWave sensor
- [x] Identify ways the mmWave sensor can be configured to reduce detection range and the non-presence timeout
- [ ] Play audio on the audio amplifier
- [ ] Play audio track when somebody enters the room


## Possible Future Development:
- [ ] LED lights when the toaster is "speaking"
- [ ] Integrate more closely with a toaster. Possibly to detect how well-done the toast is or perhaps to simply have a voice line when the toaster lever is pressed down.
- [ ] Implement "wake word detection" to detect rejections of the toasters offers of grilled bread products and respond accordingly
- [ ] Connect to WiFi to obtain the time and prevent the toaster from speaking at night. (perhaps quietly snore when a person enters the room?)
- [ ] Connect to WiFi and have the sensitivity of the motion sensor configurable.