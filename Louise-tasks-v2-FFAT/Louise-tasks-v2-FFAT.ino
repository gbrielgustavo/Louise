#include <Keypad.h> //Usada para o teclado
#include "WiFi.h" // Manipular Bluetooth e wifi
#include "FS.h"
#include "FFat.h"
#include "Arduino.h"
#include "Audio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h> //Watchdog do esp32

//#define FORMAT_FFAT true

#define I2S_LRC       26
#define I2S_BCLK      32 //25
#define I2S_DOUT      25 //33


//Manipuladores das tasks
//TaskHandle_t playAudioHandle = NULL;

String audioToPlay = "none"; //String global usada para controlar a task playAudio
String key = "none"; //String complementar para o controle

Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN); // Inicia o audio

// Parar usar o DAC interno -> Audio audio(true, I2S_DAC_CHANNEL_BOTH_EN);
// I2S_DAC_CHANNEL_RIGHT_EN /!< Enable I2S built-in DAC right channel, maps to DAC channel 1 on GPIO25/
// I2S_DAC_CHANNEL_LEFT_EN /!< Enable I2S built-in DAC left channel, maps to DAC channel 2 on GPIO26/
// I2S_DAC_CHANNEL_BOTH_EN /!< Enable both of the I2S built-in DAC channels./



// Tamanho
//Colunas e linhas teclado esquerda
const byte ROWS = 4;
const byte COLS = 4;

int volume = 21;

// Array para representar o teclado
char hexaKeys1[ROWS][COLS] = {
  {'A', 'B', 'C', 'D'},
  {'I', 'J', 'K', 'L'},
  {'Q', 'R', 'S', 'T'},
  {'Y', 'Z', '1', '2'}
};

char hexaKeys2[ROWS][COLS] = {
  {'E', 'F', 'G', 'H'},
  {'M', 'N', 'O', 'P'},
  {'U', 'V', 'W', 'X'},
  {'3', '4', '5', '6'}
};

// pins digitais onde se ligam
byte rowPins1[ROWS] = {27, 14, 12, 13}; //connect to the row pinouts of the keypad
byte colPins1[COLS] = {23, 22, 21, 19}; //connect to the column pinouts of the keypad


byte rowPins2[ROWS] = {18, 5, 17, 16}; //connect to the row pinouts of the keypad
byte colPins2[COLS] = {4, 0, 2, 15}; //connect to the column pinouts of the keypad

//criar objecto
Keypad customKeypad1 = Keypad(makeKeymap(hexaKeys1), rowPins1, colPins1, ROWS, COLS);
Keypad customKeypad2 = Keypad(makeKeymap(hexaKeys2), rowPins2, colPins2, ROWS, COLS);





void setup() {
  //disableCore0WDT(); //Desativa o Watchdog do Core 0
  disableCore1WDT(); //Desativa o Watchdog do Core 1

  btStop(); //Desativa o Bluetooth
  WiFi.mode(WIFI_OFF); //Desativa o Wifi


  Serial.begin(115200);

  if (!FFat.begin()) {
    Serial.println("FFat Mount Failed");
    return;
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(volume); // 0...21 - Ajuste do volume


  //Inicia o WD das tasks
  //esp_task_wdt_init([Intervalo em segundos], [ativa ou desativa o reset])
  esp_task_wdt_init(30, true);


  //Cria a função que irá reproduzir audio
  xTaskCreatePinnedToCore (
    playAudio, // Função a ser chamada
    "playAudio", // Nome da tarefa
    8192, // Tamanho (bytes)
    NULL, // Parametro a ser passado
    1, // Prioridade da Tarefa
    NULL, // Task handle
    1 // Núcleo que deseja rodar a tarefa (0 or 1)
  );



}



//Tarefa reproduzir audio
void playAudio(void * parameter) {

  //Inscreve a tarefa no WDT
  esp_task_wdt_add(NULL);


  while (1)
  {


    if (!audio.isRunning())
    {
      if (audioToPlay != "none") {
        String key = "/" + String(audioToPlay) + ".mp3";
        key = key.c_str();
        Serial.println(key);
        audio.connecttoFS(FFat, key.c_str(), 0);
        Serial.println("playAudio: ");
      }
    }
    else if (audio.isRunning())
    {
      while (audio.isRunning())
      {
        audio.loop();
        //if (!audio.isRunning())
        //{
        //  audioToPlay = "none";
        //}
      }
      audioToPlay = "none";
      key = "none";
      //Audio.stopSong();
      //Audio.pauseResume();
    }
    esp_task_wdt_reset();
  }
  //vTaskDelete(NULL);
}




void loop() {
  char customKey1 = customKeypad1.getKey();
  char customKey2 = customKeypad2.getKey();
  if (customKey2) {
    if (customKey2 == '5') {
      if (volume >= 6) {
        volume = volume - 5;
        audio.setVolume(volume);
      }
    }
    else if (customKey2 == '6') {
      if (volume < 21) {
        volume = volume + 5;
        audio.setVolume(volume);
      }
    }
    else {
      audioToPlay = String(customKey2);
      Serial.println(audioToPlay);
    }
  }
  else if (customKey1) {
    audioToPlay = String(customKey1);
    Serial.println(audioToPlay);
  }


}
