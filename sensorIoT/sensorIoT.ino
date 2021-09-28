#include <WiFi.h>
#include <PubSubClient.h>
#include "BluetoothSerial.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
char * id;
char* wifi_ssid;
char* wifi_password;
char* ip_broker;
int port;
int sample_time;
WiFiClient espClient;
PubSubClient client(ip_broker,port,espClient);
BluetoothSerial SerialBT;

unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;
int LED_BUILTIN = 2;

int DustAnalog = 32;
int ledPowerDust = 18;

int Config_pin = 19;

int Gas_MQ135_analog = 34;
int Gas_MQ135_digital = 4;
int Gas_MQ7_analog = 35;
int Gas_MQ7_digital = 5;
float dustSensor = 0;
float calcVoltage = 0;
float dustDensity = 0;

boolean mobile;

boolean transmitingData;


//--------------------------------------------------------------
//              Initializw NVS
//--------------------------------------------------------------
esp_err_t initializeNVS(){
  // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

}

//--------------------------------------------------------------
//              Write string on NVS
//--------------------------------------------------------------
void writeStringNVS(nvs_handle my_handle, char * key ,char * value){
        printf("Updating %s in NVS ... ",key);
        esp_err_t err = nvs_set_str(my_handle, key, value);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

}

//--------------------------------------------------------------
//              Read string on NVS
//--------------------------------------------------------------
char * readStringNVS(nvs_handle my_handle, char * key){
        printf("Reading %s from NVS ... ", key);
        size_t required_size;
        nvs_get_str(my_handle, key, NULL, &required_size);
        char* value = (char*)malloc(required_size);
        esp_err_t err = nvs_get_str(my_handle, key , value, &required_size);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Value = %s\n", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        return value;
  
}

//--------------------------------------------------------------
//              Write int32 on NVS
//--------------------------------------------------------------
void writeIntNVS(nvs_handle my_handle, char * key ,int32_t value){
        printf("Updating %s in NVS ... ",key);
        esp_err_t err = nvs_set_i32(my_handle, key, value);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");
        // Commit written value.
        // After setting any values, nvs_commit() must be called to ensure changes are written
        // to flash storage. Implementations may write to storage at other times,
        // but this is not guaranteed.
        printf("Committing updates in NVS ... ");
        err = nvs_commit(my_handle);
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

}

//--------------------------------------------------------------
//              Read int32 on NVS
//--------------------------------------------------------------
int32_t readIntNVS(nvs_handle my_handle, char * key){
        printf("Reading %s from NVS ... ", key);
        size_t required_size;
        int32_t value = 0;
        esp_err_t err = nvs_get_i32(my_handle, key, &value);
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Value = %d\n", value);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default :
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }
        return value;
  
}

//--------------------------------------------------------------
//              Write id on NVS
//--------------------------------------------------------------
void writeIdNVS(char * pId){
     // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        writeStringNVS(my_handle, "id", pId);  
        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}

//--------------------------------------------------------------
//              Write wifi info on NVS
//--------------------------------------------------------------
void writeWifiNVS(char * ssid, char * pass){
     // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        writeStringNVS(my_handle, "wifi-ssid", ssid);  
        writeStringNVS(my_handle, "wifi-pass", pass);  
        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}


//--------------------------------------------------------------
//              Write broker info on NVS
//--------------------------------------------------------------
void writeBrokerNVS(char * pIp, int32_t pPort){
     // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        writeStringNVS(my_handle, "ip-broker", pIp);  
        writeIntNVS(my_handle, "port", pPort);  
        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}

//--------------------------------------------------------------
//              Write device info on NVS
//--------------------------------------------------------------
void writeInfoNVS(int32_t st, int32_t m){
     // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        writeIntNVS(my_handle, "sample-time", st);  
        writeIntNVS(my_handle, "mobile", m);  
        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}


//--------------------------------------------------------------
//              Load config data from NVS
//--------------------------------------------------------------
void loadConfigNVS(){
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Initialize NVS
    initializeNVS();

    // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        id = readStringNVS(my_handle,"id");
        wifi_ssid = readStringNVS(my_handle,"wifi-ssid");
        wifi_password = readStringNVS(my_handle,"wifi-pass");
        ip_broker = readStringNVS(my_handle,"ip-broker");
        port = readIntNVS(my_handle,"port");
        sample_time = readIntNVS(my_handle,"sample-time");
        mobile = (readIntNVS(my_handle,"sample-time") == 1)? true : false;
        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");

 
}

//--------------------------------------------------------------
//              Connect to a wifi network
//--------------------------------------------------------------
void configureWifi(){
   printf("\n");
   WiFi.mode(WIFI_STA);
   printf("WiFi mode configurated STA \n");
   printf("Disconnecting to current network ... ");
   WiFi.disconnect();
   printf("Done \n");
   delay(100);

   printf("Connecting to %s ... ", wifi_ssid);
   WiFi.begin(wifi_ssid, wifi_password);

   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
   }
   printf("Done \n");
   
   IPAddress myIP = WiFi.localIP();
   printf("Device IP: %s \n",myIP);
  }



//--------------------------------------------------------------
//              Setup
//--------------------------------------------------------------
void setup() { 
   transmitingData= false;
   pinMode(LED_BUILTIN, OUTPUT);
   pinMode(ledPowerDust,OUTPUT);
   Serial.begin(115200);
   loadConfigNVS();
   configureWifi();  
   client.setServer("test.mosquitto.org", 1883);
}


//--------------------------------------------------------------
//              Verify connection with broker
//--------------------------------------------------------------
void verifyBrokerConnection(){
  if (!client.connected()) {
    transmitingData = false;
    Serial.println("Conectando con cliente");
      if (client.connect("CUPCARBON-ID")) {
        transmitingData = true;
         digitalWrite(LED_BUILTIN, 1);
         delay(100);
         digitalWrite(LED_BUILTIN, 0);
         delay(100);
         digitalWrite(LED_BUILTIN, 1);
         delay(100);
         digitalWrite(LED_BUILTIN, 0);
         delay(100);
      } 
   }
  
  }



//--------------------------------------------------------------
//              Read and publish all sensor data
//--------------------------------------------------------------
void readSensors(){
  int gassensorMq7Analog = analogRead(Gas_MQ7_analog);
  int gassensorMq7Digital = digitalRead(Gas_MQ7_digital);
  int gassensorMq135Analog = analogRead(Gas_MQ135_analog);
  int gassensorMq135Digital = digitalRead(Gas_MQ135_digital);
  


  digitalWrite(ledPowerDust,LOW);
  delayMicroseconds(samplingTime);

  dustSensor = analogRead(DustAnalog);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPowerDust,HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = dustSensor*(5.0/1024);
  dustDensity = 0.17*calcVoltage-0.1;

  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }
  //----
   char cstr[16];
   client.publish("AQ/ESP32/CO",itoa(gassensorMq7Analog, cstr, 10) );
   client.publish("AQ/ESP32/CO2",itoa(gassensorMq135Analog, cstr, 10) );
   client.publish("AQ/ESP32/DUST",itoa((int)dustSensor, cstr, 10) );
   delay(sample_time);
  }

//--------------------------------------------------------------
//              Blink LED for config mode
//--------------------------------------------------------------
boolean blinkConfig(){

  for (int i=1;i<=10;i++){
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
    digitalWrite(LED_BUILTIN, LOW);
    if(digitalRead(Config_pin) == LOW){
      return false;
      }
    }
    return true;
  
  }

void callback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  if(event == ESP_SPP_SRV_OPEN_EVT){
    Serial.println("BT Client Connected");
  }
 
  if(event == ESP_SPP_CLOSE_EVT ){
    Serial.println("BT Client disconnected");
  }
}


//--------------------------------------------------------------
//              Split string
//--------------------------------------------------------------
 String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}


//--------------------------------------------------------------
//              Config mode 
//--------------------------------------------------------------
void configModule(){
  SerialBT.register_callback(callback);
  SerialBT.begin("IOT-AQ-ID");
  Serial.println("The device started, now you can pair it with bluetooth!");
  boolean finished = false;
      while(!finished){
        String option = readBTMSJ();
        finished = processBT(option);      
      }
}

//--------------------------------------------------------------
//              Process command BT modifiers
//--------------------------------------------------------------
void processModifiersBT(String option){
  
  // copying the contents of the
  // string to char array
  String token = getValue(option,':',1);
  if(token!= NULL){              
    String value = getValue(option,':',0);
    char i[50];
    value.toCharArray(i, 50);
    
    char* info;
    info = (char*)malloc(50); 
    strcpy(info, i);
    
    if(String(token) == "ID\r\n"){
      id = info;  
      writeIdNVS(id); 
    } else if (String(token)=="IP\r\n"){
      ip_broker = info;   
      writeBrokerNVS(ip_broker, port);
      
    } else if (String(token)=="WIFI-SSID\r\n"){
      wifi_ssid = info;
      writeWifiNVS(wifi_ssid, wifi_password);
    } else if (String(token)=="WIFI-PASS\r\n"){
      wifi_password= info;
      writeWifiNVS(wifi_ssid, wifi_password);
    } else if (String(token)=="PORT\r\n"){
      int i;
      sscanf(info, "%d", &i);
      port= i;
      writeBrokerNVS(ip_broker, port);
    } else {
      
    }

  }
}

//--------------------------------------------------------------
//              Process command BT
//--------------------------------------------------------------
boolean processBT(String option){
    if(option == "MOBILE\r\n"){
        mobile=true;   
    } else if (option == "STATIC\r\n"){
        mobile=false;
    } else if (option == "EXIT\r\n"){
        SerialBT.println("OK");
        return true;
    } else if (option == "RESTART\r\n"){
        SerialBT.println("OK");
        ESP.restart();
    } else if (option == "HELLO\r\n"){
        SerialBT.println("HELLO");
        SerialBT.println("ID:"+String(id));
        SerialBT.println("WIFI-SSID:"+String(wifi_ssid));
        SerialBT.println("WIFI-PASS:"+String(wifi_password));
        SerialBT.println("IP-BROKER:"+String(ip_broker));
        SerialBT.println("PORT:"+String(port));
        SerialBT.println("MOBILE:"+String(mobile));
        SerialBT.println("OK");
    } else {
        processModifiersBT(option);  
    }
    SerialBT.println("OK");
    return false;
  
}


//--------------------------------------------------------------
//             Read BT serial Message
//--------------------------------------------------------------
String readBTMSJ(){
  while(!SerialBT.available()){
    }
  return SerialBT.readString();
  }


//--------------------------------------------------------------
//              Loop
//--------------------------------------------------------------
void loop() {
  digitalWrite(LED_BUILTIN, LOW);
  int Push_button_state = digitalRead(Config_pin);
  if ( Push_button_state == HIGH )
    { 
      if(blinkConfig()){   
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print("Entrando a modo configuracion...");
    configModule();
        }
    }
    else 
    {
     verifyBrokerConnection();
     if (transmitingData){
      readSensors();
    
      }
     }
   client.loop();
}
