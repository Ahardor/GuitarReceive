#define CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED 0
#define CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED 0

#include <esp_now.h>
#include <esp_wifi.h>
#include <driver/dac.h>
#include <esp32-hal-timer.h>
#include <HardwareSerial.h>

uint8_t input_buffer[250];
uint8_t output_buffer[250];

volatile uint8_t i=0;
volatile uint8_t data=0;
volatile uint8_t data_input=0;

hw_timer_t *My_timer = NULL;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(input_buffer, incomingData, sizeof(input_buffer));
  // Serial.println("Received Data: ");
  data_input=1;
}

void IRAM_ATTR writeDAC(){
  BaseType_t xHigherPriorityTaskWoken;
  
  if(i==0 && data_input){ 
    memcpy(output_buffer, input_buffer, sizeof(output_buffer));
    data_input=0;
    data=1;
  }
  if(data){
    dac_output_voltage(DAC_CHANNEL_1, output_buffer[i++]);
    if(i==250){
        i=0;
        data=0;
    }
  }
}
 
void setup() {
  Serial.begin(115200);
  dac_output_enable(DAC_CHANNEL_1);

  esp_netif_init();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_bandwidth(WIFI_IF_STA, WIFI_BW_HT40);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_start();
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);

  My_timer = timerBegin(3, 8, true);
  timerAttachInterrupt(My_timer, &writeDAC, true);
  timerAlarmWrite(My_timer, 500, true);
  timerAlarmEnable(My_timer);
  
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
}