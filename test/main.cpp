#include <esp_now.h>
#include <WiFi.h>
#include <driver/dac_common.h>
#include <esp_wifi.h>

uint8_t broadcastAddress[] = {0xE4, 0x65, 0xB8, 0x10, 0xDC, 0x78};

esp_now_peer_info_t peerInfo;

uint8_t IN_BUFF[250];
uint8_t OUT_BUFF[250];

volatile uint8_t pos=0;
volatile uint8_t data4dac=0;
volatile uint8_t data_rec=0;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  BaseType_t xHigherPriorityTaskWoken;
  memcpy(IN_BUFF, incomingData, sizeof(IN_BUFF));
  data_rec=1;
}

void IRAM_ATTR writeDAC(){
  BaseType_t xHigherPriorityTaskWoken;
  if(pos==0 && data_rec){ 
    //if new data has been received and OUT_BUFF is free to use, copy in the new data from IN_BUFF, set data4dac to 1 and reset data_rec
    memcpy(OUT_BUFF, IN_BUFF, sizeof(OUT_BUFF));
    data_rec=0;
    data4dac=1;
  }
  if(data4dac){
    Serial.println( OUT_BUFF[pos]);
    dac_output_voltage(DAC_CHANNEL_1, OUT_BUFF[pos++]);
    if(pos==250){
        //if finished sending 250 bytes, reset counter and data4dac
        pos=0;
        data4dac=0;
    }
  }
}
 
void setup() {
  Serial.begin(115200);
  dac_output_enable(DAC_CHANNEL_1);
  
  // Set device as a Wi-Fi Station
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
  else {
    Serial.println("ESPNow Init Success");
  }
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_54M);
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  
  writeDAC();
  
  // Serial.println(SoundIncome.sound[pos]);
  delayMicroseconds(125);
}
