#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"


URLStream url("ssid","password");
I2SStream i2s; // final output of decoded stream
EncodedAudioStream dec(&i2s, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier(dec, url); // copy url to decoder


void setup() {
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);  

  // setup i2s
  auto config = i2s.defaultConfig(TX_MODE);
  i2s.begin(config);

  // setup I2S based on sampling rate provided by decoder
  dec.begin();

  // mp3 radio
  url.begin("http://stream.srg-ssr.ch/m/rsj/mp3_128","audio/mp3");
}

void loop(){
  copier.copy();
}
