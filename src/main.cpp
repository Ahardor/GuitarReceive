#define CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED 0
#define CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED 0

#include <Wifi.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <esp32-hal-timer.h>
#include <PubSubClient.h>

const int buffer_size = 250;

uint8_t input_buffer [buffer_size];
uint8_t output_buffer[buffer_size];

volatile int pos=0;
volatile uint8_t data=0;
volatile uint8_t data_input=0;


hw_timer_t *My_timer = NULL;

const char* ssid = "RockusWIFI";
const char* password = "Alvard86";

const char* address = "192.168.1.109";
const int port = 1883;
const char* topic = "sound_out";

WiFiClient espClient;
PubSubClient client(espClient);


void IRAM_ATTR writeDAC(){
    BaseType_t xHigherPriorityTaskWoken;
    if(pos==0 && data_input){ 
        memcpy(output_buffer, input_buffer, buffer_size);
        data_input=0;
        data=1;
    }
    if(data){
        dac_output_voltage(DAC_CHANNEL_1, output_buffer[pos++]);
        if(pos >= buffer_size){
            pos=0;
            data=0;
        }
    }
    
}

void callbackFunction(char* topic, byte* payload, unsigned int length) {
    BaseType_t xHigherPriorityTaskWoken;
    memcpy(input_buffer, payload, buffer_size);
    data_input=1;
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
    
    client.setBufferSize(buffer_size + 20);
    client.setServer(address, port);
    client.setCallback(callbackFunction);

    My_timer = timerBegin(3, 8, true);
    timerAttachInterrupt(My_timer, &writeDAC, true);
    timerAlarmWrite(My_timer, 625, true);
    timerAlarmEnable(My_timer);

}

unsigned long last = 0;

void loop() {
    if (!client.connected()) {
            while (!client.connected()) {
                Serial.print("MQTT connecting ...");
                String clientId = "Receiver";
                if (client.connect(clientId.c_str())) {
                    Serial.println("connected");
                    client.subscribe(topic);
                    
                } else {
                    Serial.print("failed, status code =");
                    Serial.print(client.state());
                    Serial.println("try again in 5 seconds");
                    /* Wait 5 seconds before retrying */
                    delay(5000);
                }
            }
            
        }

    if(micros() - last >= 15625){
        last = micros();
        client.loop();
    }
    
}