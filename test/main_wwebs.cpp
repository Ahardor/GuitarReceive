#define CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED 0
#define CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED 0

#include <Wifi.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <esp32-hal-timer.h>
#include <WebSocketsClient.h>
#include <WebServer.h>

const int buffer_size = 500;

byte input_buffer[buffer_size];
byte output_buffer[buffer_size];

volatile uint8_t pos=0;
volatile uint8_t data=0;
volatile uint8_t data_input=0;

hw_timer_t *My_timer = NULL;

const char* ssid = "RockusWIFI";
const char* password = "Alvard86";

const char* address = "192.168.1.109";
const int port = 8080;
const char* path = "/ws";


WebSocketsClient webSocket;

void IRAM_ATTR writeDAC(){
    BaseType_t xHigherPriorityTaskWoken;
    if(pos==0 && data_input){ 
        memcpy(output_buffer, input_buffer, buffer_size);
        data_input=0;
        data=1;
    }
    if(data){
        dac_output_voltage(DAC_CHANNEL_1, output_buffer[pos++]);
        if(pos == buffer_size){
            pos=0;
            data=0;
        }
    }
}

void OnDataRecv(WStype_t type, uint8_t * payload, size_t length) {
    BaseType_t xHigherPriorityTaskWoken;
    if(type == WStype_BIN){
        memcpy(input_buffer, payload, buffer_size);
        // for(int i = 0; i < 249; i++) {
        //     Serial.printf("%d ", frame.data[0][i]);
        // }

        // Serial.println("\n");
        data_input=1;
    }
}

void setup() {
    Serial.begin(115200);
    dac_output_enable(DAC_CHANNEL_1);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }

    webSocket.setExtraHeaders("Origin: Receiver");
    webSocket.begin(address, port, path);

    webSocket.setReconnectInterval(5000);

    webSocket.onEvent(OnDataRecv);

    My_timer = timerBegin(3, 8, true);
    timerAttachInterrupt(My_timer, &writeDAC, true);
    timerAlarmWrite(My_timer, 1000, true);
    timerAlarmEnable(My_timer);

}

void loop() {
    webSocket.loop();
}