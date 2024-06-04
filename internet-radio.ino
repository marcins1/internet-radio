#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <string>
#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"

const char* configurationPage = R"rawliteral(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Configuration</title>
  </head>
  <style>
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }

    body {
      background-color: #bdbdbd;
      font-family: Verdana, Geneva, Tahoma, sans-serif;
    }

    .container {
      width: 100vw;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
    }

    .wrapper {
      background-color: #f4f5f7;
      padding: 0 0 1rem;
      margin: 1rem;
    }

    .header {
      background: #ffffff;
      text-align: center;
      font-size: 1.25rem;
      font-weight: 600;
      padding: 1rem;
      margin: 0 0 1rem;
    }

    label {
      display: block;
      margin: 0 0 0.4rem;
      font-weight: 600;
      font-size: 0.9rem;
    }

    input {
      padding: 0.7rem;
      border-radius: 4px;
      outline: 0;
      font-size: 1rem;
      width: 100%;
      border: 1px solid #2ca838;
      margin: 0 0 0.5rem;
    }

    button {
      padding: 0.75rem 1.5rem;
      margin: 0.5rem 0 0;
      outline: 0;
      border: 0;
      background: #2ca838;
      border-radius: 4px;
      color: #ffffff;
      font-weight: 700;
      font-size: 0.9rem;
      transition: background 0.4s;
    }

    .buttons {
      display: flex;
      flex-wrap: wrap;
      align-items: center;
      justify-content: center;
      gap: 0.5rem;
    }

    button:hover {
      background: rgb(76, 206, 89);
    }

    form {
      padding: 0 1rem;
    }
  </style>
  <body>
    <div class="container">
      <div class="wrapper">
        <div class="header">
          <span>ESP32 Internet Radio Configuration</span>
        </div>
        <form id="configurationForm">
          <label for="ssid">Wi-Fi network name</label>
          <input type="text" id="ssid" name="ssid" required />
          <label for="password">Wi-Fi password</label>
          <input type="password" id="password" name="password" required />
          <div id="inputContainer"></div>
          <div class="buttons">
            <button type="button" onclick="addInput()">Add radio</button>
            <button type="button" onclick="removeInput()">Delete radio</button>
            <button type="button" onclick="loadConfiguration()">Load</button>
            <button type="submit">Save</button>
          </div>
        </form>
      </div>
    </div>
    <script>
      let counter = 1;
      const inputContainer = document.getElementById("inputContainer");
      const networkName = document.getElementById("ssid");
      const password = document.getElementById("password");

      document
        .getElementById("configurationForm")
        .addEventListener("submit", function (event) {
          event.preventDefault();

          const inputs = inputContainer.querySelectorAll("input");
          const radios = [];

          inputs.forEach((input) => {
            radios.push(input.value);
          });

          const urlWithParams = `http://192.168.1.1/new?ssid=${
            networkName.value
          }&password=${password.value}&nradios=${
            radios.length
          }&radios=${radios.join(",")}`;

          fetch(urlWithParams, {
            method: "POST",
          })
            .then(() => {
              alert("Configuration saved successfully!");
            })
            .catch((error) => {
              console.error("Error:", error);
              alert("An error occurred while saving the configuration!");
            });
        });

      function addInput() {
        const newLabel = document.createElement("label");
        newLabel.htmlFor = `radio${counter}`;
        newLabel.textContent = `Radio ${counter}`;
        inputContainer.appendChild(newLabel);

        const newInput = document.createElement("input");
        newInput.type = "text";
        newInput.placeholder = "Radio stream link";
        newInput.required = true;
        inputContainer.appendChild(newInput);

        counter++;
      }

      function removeInput() {
        if (counter > 2) {
          const inputs = inputContainer.querySelectorAll("input");
          const lastInput = inputs[inputs.length - 1];
          const lastLabel = lastInput.previousElementSibling;

          inputContainer.removeChild(lastInput);
          inputContainer.removeChild(lastLabel);

          counter--;
        }
      }

      function clearInputs() {
        while (inputContainer.hasChildNodes()) {
          inputContainer.removeChild(inputContainer.firstChild);
        }
        counter = 1;
      }

      function loadConfiguration() {
        fetch("http://192.168.1.1/configuration", {
          method: "GET",
        })
          .then((response) => response.json())
          .then((data) => {
            if (data.radios.length > 0) {
              clearInputs();
              networkName.value = data.ssid;
              password.value = data.password;
              data.radios.forEach((radio) => {
                addInput();
                document.querySelector("input:last-child").value = radio;
              });
              setTimeout(() => {
                alert("Configuration loaded successfully!");
              }, 100);
            } else {
              alert("No configuration found!");
            }
          })
          .catch((error) => {
            console.error("Error:", error);
            alert("An error occurred while loading the configuration!");
          });
      }

      addInput();
    </script>
  </body>
</html>
)rawliteral";

#define BUTTON_PIN1 5
#define BUTTON_PIN2 18
#define BUTTON_PIN3 19
#define BUTTON_PIN4 21
#define BUTTON_PIN5 3

const char* ap_ssid = "ESP32 Internet Radio";
const char* ap_password = "1234567890";

IPAddress IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
AsyncWebServer server(80);

std::vector<std::string> radiosVector;
int numberOfRadios = 0;
int currentStationIndex = 0;

bool configurationMode = true;
bool radioConfigured = false;

char* wifi_ssid = "";
char* wifi_password = "";

URLStream url("UNKNOWN", "UNKNOWN");
I2SStream i2s; // final output of decoded stream
VolumeStream volume(i2s);
EncodedAudioStream dec(&volume, new MP3DecoderHelix()); // Decoding stream
StreamCopy copier(dec, url); // copy url to decoder

bool buttonAlreadyPressed1 = false;
bool buttonAlreadyPressed2 = false;
bool buttonAlreadyPressed3 = false;
bool buttonAlreadyPressed4 = false;
bool buttonAlreadyPressed5 = false;
int buttonState = 0;

float currentVolumeLevel = 1;

char* stringToCharArray(String str) {
  int length = str.length() + 1;
  char* charArray = new char[length];
  str.toCharArray(charArray, length);
  return charArray;
}

void APTurnOn() {
  bool ap_started = WiFi.softAP(ap_ssid, ap_password);
  if (!ap_started) {
    Serial.println("Failed to start AP!");
    return;
  }
  WiFi.softAPConfig(IP, gateway, subnet);
}

void setup() {
  Serial.begin(115200);
  // AudioLogger::instance().begin(Serial, AudioLogger::Info);  

  WiFi.mode(WIFI_AP);
  APTurnOn();

  server.on("/new", HTTP_POST, [](AsyncWebServerRequest *request) {
    char* newSSID = stringToCharArray(request->getParam("ssid")->value());
    char* newPassword = stringToCharArray(request->getParam("password")->value());
    char* numberOfRadiosString = stringToCharArray(request->getParam("nradios")->value());
    char* token;
    char* radios = stringToCharArray(request->getParam("radios")->value());
    token = strtok(radios, ",");
    radiosVector.clear();
    while (token != NULL) {
      radiosVector.push_back(std::string(token));
      token = strtok(NULL, ",");
    }
    wifi_ssid = newSSID;
    wifi_password = newPassword;
    url.setSSID(newSSID);
    url.setPassword(newPassword);
    numberOfRadios = atoi(numberOfRadiosString);
    radioConfigured = true;
    currentStationIndex = 0;
    Serial.println("New radio configuration saved");
    request->send(200, "text/plain", "Settings updated");
  });

  server.on("/configuration", HTTP_GET, [](AsyncWebServerRequest *request) {
    JsonDocument doc;
    doc["ssid"] = wifi_ssid;
    doc["password"] = wifi_password;
    JsonArray array = doc.createNestedArray("radios");
    for (int i = 0; i < radiosVector.size(); i++) {
      array.add(radiosVector[i].c_str());
    }
    String jsonString;
    serializeJson(doc, jsonString);
    request->send_P(200, "application/json", jsonString.c_str());
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", configurationPage);
  });

  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);
  pinMode(BUTTON_PIN4, INPUT_PULLUP);
  pinMode(BUTTON_PIN5, INPUT_PULLUP);

  server.begin();
}

void volumeDown() {
    if (currentVolumeLevel != 0) {
      currentVolumeLevel -= 1;
      volume.setVolume(currentVolumeLevel / 10);
      Serial.println("VOLUME DOWN - " + String(currentVolumeLevel / 10));
    }
}

void volumeUp() {
    if (currentVolumeLevel != 10) {
      currentVolumeLevel += 1;
      volume.setVolume(currentVolumeLevel / 10);
      Serial.println("VOLUME UP - " + String(currentVolumeLevel / 10));
    }
}

void changeStation() {
  url.end();
  url.begin(radiosVector[currentStationIndex].c_str());
  Serial.println("STATION SET TO - " + String(radiosVector[currentStationIndex].c_str()));
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

I2SConfig getConfig() {
  I2SConfig config = i2s.defaultConfig(TX_MODE);
  config.pin_bck = 15;
  config.pin_ws = 14;
  config.pin_data = 22;
  config.i2s_format = I2S_LSB_FORMAT;
  return config;
}

void modeSwitch() {
  if (radioConfigured) {
    configurationMode = !configurationMode;
    if (configurationMode) {
      Serial.println("Entered configuration mode");
      url.end();
      volume.end();
      dec.end();
      i2s.end();
      APTurnOn();
      server.begin();
    } else {
      Serial.println("Entered radio mode");
      auto config = getConfig();
      server.end();
      WiFi.softAPdisconnect(true);
      i2s.begin(config);
      dec.begin();
      volume.begin(config);
      currentVolumeLevel = 1;
      volume.setVolume(currentVolumeLevel / 10);
      url.begin(radiosVector[currentStationIndex].c_str(), "audio/mp3");
    }
  } else {
    Serial.println("Radio must be configured first!");
  }
}

void loop() {
  if (!configurationMode) {
    copier.copy();
    handleButton(BUTTON_PIN1, volumeDown, &buttonAlreadyPressed1);
    handleButton(BUTTON_PIN2, volumeUp, &buttonAlreadyPressed2);
    handleButton(BUTTON_PIN3, stationLeft, &buttonAlreadyPressed3);
    handleButton(BUTTON_PIN4, stationRight, &buttonAlreadyPressed4);
  }
  handleButton(BUTTON_PIN5, modeSwitch, &buttonAlreadyPressed5);
}
