#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"

#define BUTTON_PIN1 5
#define BUTTON_PIN2 18
#define BUTTON_PIN3 19
#define BUTTON_PIN4 21

URLStream url("ssid","password");
I2SStream i2s; // final output of decoded stream
VolumeStream volume(i2s);
EncodedAudioStream dec(&volume, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier(dec, url); // copy url to decoder

bool buttonAlreadyPressed1 = false;
bool buttonAlreadyPressed2 = false;
bool buttonAlreadyPressed3 = false;
bool buttonAlreadyPressed4 = false;
int buttonState = 0;

char* radios[] = {"http://stream.srg-ssr.ch/m/rsj/mp3_128", "http://195.150.20.242:8000/rmf_maxxx", "https://waw.ic.smcdn.pl/2070-1.mp3", "https://stream.radioparadise.com/mp3-128", "https://radiostream.pl/tuba139-1.mp3"};
int numberOfRadios = sizeof(radios) / sizeof(radios[0]);
int currentStationIndex = 0;

float volumeValues[] = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7};
int numberOfVolumeValues = sizeof(volumeValues) / sizeof(volumeValues[0]);
int currentVolumeIndex = 0;

void setup() {
  Serial.begin(115200);
  // AudioLogger::instance().begin(Serial, AudioLogger::Info);  

  // setup i2s
  auto config = i2s.defaultConfig(TX_MODE);
  config.pin_bck = 15;
  config.pin_ws = 14;
  config.pin_data = 22;
  config.i2s_format = I2S_LSB_FORMAT;
  i2s.begin(config);

  // setup I2S based on sampling rate provided by decoder
  dec.begin();

  volume.begin(config);
  volume.setVolume(volumeValues[currentVolumeIndex]);

  // mp3 radio
  url.begin(radios[currentStationIndex],"audio/mp3");

  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);
  pinMode(BUTTON_PIN4, INPUT_PULLUP);
}

void volumeDown() {
    if (currentVolumeIndex != 0) {
      currentVolumeIndex = currentVolumeIndex - 1;
      volume.setVolume(volumeValues[currentVolumeIndex]);
      Serial.println("VOLUME DOWN - " + String(volumeValues[currentVolumeIndex]));
    }
}

void volumeUp() {
    if (currentVolumeIndex != numberOfVolumeValues - 1) {
      currentVolumeIndex = currentVolumeIndex + 1;
      volume.setVolume(volumeValues[currentVolumeIndex]);
      Serial.println("VOLUME UP - " + String(volumeValues[currentVolumeIndex]));
    }
}

void changeStation() {
  url.end();
  url.begin(radios[currentStationIndex]);
  Serial.println("STATION SET TO - " + String(radios[currentStationIndex]));
}

void stationLeft() {
  if (currentStationIndex == 0) {
    currentStationIndex = numberOfRadios - 1;
  } else {
    currentStationIndex = currentStationIndex - 1;
  }
  changeStation();
}

void stationRight() {
  currentStationIndex = (currentStationIndex + 1) % numberOfRadios;
  changeStation();
}

void handleButton(int button, void (*handler)(void), bool* buttonAlreadyPressed) {
  buttonState = digitalRead(button);
  if (buttonState == 1 && *buttonAlreadyPressed == false) {
    *buttonAlreadyPressed = true;
    handler();
  } else if (buttonState == 0) {
    *buttonAlreadyPressed = false;
  }
}

void loop(){
  copier.copy();
  handleButton(BUTTON_PIN1, volumeDown, &buttonAlreadyPressed1);
  handleButton(BUTTON_PIN2, volumeUp, &buttonAlreadyPressed2);
  handleButton(BUTTON_PIN3, stationLeft, &buttonAlreadyPressed3);
  handleButton(BUTTON_PIN4, stationRight, &buttonAlreadyPressed4);
}
