#include <WiFi.h>
#include <PubSubClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <HTTPClient.h>
#include "BluetoothSerial.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "FS.h"
#include "SPIFFS.h"
#include "time.h"
#include "MQ135.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"


#define FORMAT_SPIFFS_IF_FAILED true
char * id;
char* wifi_ssid;
char* wifi_password;
char* ip_broker;
int port;
int sample_time;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5*3600;
const int   daylightOffset_sec = 3600;
char fileCO[30];
char fileCO2[30];
char filePM[30];

//MQ7
float RS_gas = 0;

float ratio = 0;

float sensorValue = 0;

float sensor_volt = 0;

float R0 = 7200.0;

//MQ135
float sensorValueMQ135 = 0;
float sensor_voltMQ135 = 0;
float a = 5.5973021420;
float b = -0.365425824;
float R0_MQ135 = 7200.0;
float R_L = 20000.0;


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
MQ135 gasSensor = MQ135(Gas_MQ135_analog);
boolean mobile;
float ppmMQ135 = 0;

boolean transmitingData;


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
//              Write wifi SSID on NVS
//--------------------------------------------------------------
void writeWifiSsidNVS(char * ssid){
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

        // Close
        nvs_close(my_handle);
    }

    printf("\n");
}
//--------------------------------------------------------------
//              Write wifi info on NVS
//--------------------------------------------------------------
void writeWifiPassNVS(char * pass){
     // Open
    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");
    nvs_handle my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

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
//              SPIFFS
//--------------------------------------------------------------


void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}



void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\r\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("- failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("- message appended");
    } else {
        Serial.println("- append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\r\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("- file renamed");
    } else {
        Serial.println("- rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}


void clearDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Clearing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
            deleteFile(SPIFFS, file.name());
        }
        file = root.openNextFile();
    }
}

void testFileIO(fs::FS &fs, const char * path){
    Serial.printf("Testing file I/O with %s\r\n", path);

    static uint8_t buf[512];
    size_t len = 0;
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }

    size_t i;
    Serial.print("- writing" );
    uint32_t start = millis();
    for(i=0; i<2048; i++){
        if ((i & 0x001F) == 0x001F){
          Serial.print(".");
        }
        file.write(buf, 512);
    }
    Serial.println("");
    uint32_t end = millis() - start;
    Serial.printf(" - %u bytes written in %u ms\r\n", 2048 * 512, end);
    file.close();

    file = fs.open(path);
    start = millis();
    end = start;
    i = 0;
    if(file && !file.isDirectory()){
        len = file.size();
        size_t flen = len;
        start = millis();
        Serial.print("- reading" );
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            if ((i++ & 0x001F) == 0x001F){
              Serial.print(".");
            }
            len -= toRead;
        }
        Serial.println("");
        end = millis() - start;
        Serial.printf("- %u bytes read in %u ms\r\n", flen, end);
        file.close();
    } else {
        Serial.println("- failed to open file for reading");
    }
}


void configFiles(){
  struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }   
  strftime(fileCO,30, "/CO#%Y-%m-%d.txt", &timeinfo);
  strftime(fileCO2,30, "/CO2#%Y-%m-%d.txt", &timeinfo);
  strftime(filePM,30, "/PM#%Y-%m-%d.txt", &timeinfo);
  //Check if files exist
  if (!SPIFFS.exists(fileCO)){
    writeFile(SPIFFS, fileCO, "CO data\r\n");  
  }
  if (!SPIFFS.exists(fileCO2)){
    writeFile(SPIFFS, fileCO2, "CO2 data\r\n");
  }
  if (!SPIFFS.exists(filePM)){
    writeFile(SPIFFS, filePM, "PM data\r\n"); 
  } 
}

void storeData(char * coValue, char * co2Value, char * pmValue ){
  struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
  char hour[9];
  strftime(hour,9, "%H:%M:%S", &timeinfo);

  char lineCO [16];
  strcpy(lineCO,hour);
  strcat(lineCO,"-");
  strcat(lineCO,coValue);
  strcat(lineCO,"\r\n");

  char lineCO2 [16];
  strcpy(lineCO2,hour);
  strcat(lineCO2,"-");
  strcat(lineCO2,co2Value);
  strcat(lineCO2,"\r\n");

  char linePM [16];
  strcpy(linePM,hour);
  strcat(linePM,"-");
  strcat(linePM,pmValue);
  strcat(linePM,"\r\n");
 
  appendFile(SPIFFS, fileCO, lineCO);
  appendFile(SPIFFS, fileCO2, lineCO2);
  appendFile(SPIFFS, filePM, linePM);

}

//--------------------------------------------------------------
//              Upload to server the stored Data and erase files if completed
//--------------------------------------------------------------
void uploadStoredData(){
  HTTPClient httpClient;
  char * dirname = "/";
  uint8_t levels = 0;
  fs::FS &fs = SPIFFS;
  Serial.println("Uploading previous data");
  Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
            Serial.printf("Reading file: %s\r\n", file.name());
            if(!file || file.isDirectory()){
                Serial.println("- failed to open file for reading");
                return;
            }
        
            Serial.println("- read from file:");
            String fileWithoutExtension;
            String varFile;
            String variable;
            
            String day;
            fileWithoutExtension =  getValue(file.name(),'.',0);
            varFile =  getValue(fileWithoutExtension,'#',0);
            day =  getValue(fileWithoutExtension,'#',1);
    
            

            if(varFile.equals("/CO")){
              variable="1";
            } else if(varFile.equals("/CO2")){
                variable="2";
            } else if(varFile.equals("/PM")){
               variable="3";
            } else{
               variable="4";
            }
            
            
         
            String base = "http://35.211.127.202:3000/measurement?device="+(String)id +"&variable="+variable+"&timestamp="+day+"T";
            String linea;
            String hora;
            String valor;
            String request;
            
      while(file.available()){
              linea=file.readStringUntil('\n');
              hora = getValue(linea,'-',0);
              valor = getValue(linea,'-',1);
              valor.trim();
              if(valor != NULL){
                request = base + hora + "&value=" + valor;
                Serial.println(request);
                delay(100);
                httpClient.begin(request.c_str());
                int httpResponseCode = httpClient.POST("");
                Serial.println(String(httpResponseCode));        
                }
              
              
            }
            file.close();
            
        }
        file = root.openNextFile();
    }
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
   //Updates time for the esp32
   printLocalTime();
   configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   printLocalTime();
  }
//--------------------------------------------------------------
//              Keep wifi alive
//--------------------------------------------------------------
void keepWifiAlive(void * parameters){
  for(;;){
    if(WiFi.status()==WL_CONNECTED){
        vTaskDelay(10000/portTICK_PERIOD_MS);
        continue;
      }
      //
      printf("\n");
     WiFi.mode(WIFI_STA);
     printf("WiFi mode configurated STA \n");
     printf("Disconnecting to current network ... ");
     WiFi.disconnect();
     printf("Done \n");
   
  
     printf("Connecting to %s ... ", wifi_ssid);
     WiFi.begin(wifi_ssid, wifi_password);
  
     while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(10000/portTICK_PERIOD_MS);
        continue;
     }
     printf("Done \n");
     
     IPAddress myIP = WiFi.localIP();
     printf("Device IP: %s \n",myIP);
     //Updates time for the esp32
     configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
     printLocalTime();
     uploadStoredData();
     clearDir(SPIFFS, "/", 0);
     configFiles();
     listDir(SPIFFS, "/", 0);
   

    }
  
  }


//--------------------------------------------------------------
//              Blink wifi not connected
//--------------------------------------------------------------
void blinkNoConnection(void * parameters){
  for(;;){
  if(WiFi.status()==WL_CONNECTED){ 
    vTaskDelay(10000/portTICK_PERIOD_MS);
    continue;
  }
//   Serial.println("Desonectadisimo");
   
    digitalWrite(LED_BUILTIN, HIGH);
    vTaskDelay(500/portTICK_PERIOD_MS);
    digitalWrite(LED_BUILTIN, LOW);
    vTaskDelay(500/portTICK_PERIOD_MS);}
    
}


//--------------------------------------------------------------
//              BLE CHARACTERISTICS
//--------------------------------------------------------------
#define SERVICE_UUID        "0000180A-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_NAME_UUID "00002A00-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_ID_UUID "00002A23-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_SWV "00002A28-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_ALERT "00002A3F-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_TYPE "00003000-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_LAT "00002AAE-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_LONG "00002AAF-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_WIFI_SSID "00003001-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_WIFI_PASS "00003002-0000-1000-8000-00805F9B34FB"
#define CHARACTERISTIC_UUID_WIFI_PASS2 "00004002-0000-1000-8000-00805F9B34FB"


BLECharacteristic *pCharacteristicName;
BLECharacteristic *pCharacteristicID;
BLECharacteristic *pCharacteristicSWV;
BLECharacteristic *pCharacteristicLat;
BLECharacteristic *pCharacteristicLong;
BLECharacteristic *pCharacteristicAlert;
BLECharacteristic *pCharacteristicType;

BLECharacteristic *pCharacteristicSSID;
BLECharacteristic *pCharacteristicPASS;
BLECharacteristic *pCharacteristicPASS2;

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      String dataRecibed;
      if (rxValue.length() > 0) {
        for (int i = 0; i < rxValue.length(); i++){
          dataRecibed+=rxValue[i];
          }
      }
      String value = dataRecibed;
      char i[50];
      value.toCharArray(i, 50);
  
      char* info;
      info = (char*)malloc(50); 
      strcpy(info, i);
      if ( (pCharacteristic->getUUID()).equals(BLEUUID(CHARACTERISTIC_ID_UUID))  ){
        id = info;  
        writeIdNVS(id); 
        }

      if ( (pCharacteristic->getUUID()).equals(BLEUUID(CHARACTERISTIC_UUID_WIFI_SSID))  ){
        wifi_ssid = info;  
        writeWifiSsidNVS(wifi_ssid); 
        }

      if ( (pCharacteristic->getUUID()).equals(BLEUUID(CHARACTERISTIC_UUID_WIFI_PASS))  ){
        wifi_password = info;  
        writeWifiPassNVS(wifi_password); 
        delay(5000);
        configureWifi();
        }
      
      
    }
};
//--------------------------------------------------------------
//              LOAD BLE CHARACTERISTICS
//--------------------------------------------------------------
void initializeCharacteristics(){
  BLEDevice::init("AQ-IoT Node");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristicName = pService->createCharacteristic(
                                         CHARACTERISTIC_NAME_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristicID = pService->createCharacteristic(
                                         CHARACTERISTIC_ID_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );


  pCharacteristicLat = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_LAT,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristicLong = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_LONG,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristicSSID = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_WIFI_SSID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristicPASS = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_WIFI_PASS,
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
 pCharacteristicPASS2 = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_WIFI_PASS2,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
                                       

  pCharacteristicType = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_TYPE,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristicAlert = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_ALERT,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristicSWV = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_SWV,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristicName->setValue("Sensor calidad aire");
  pCharacteristicName->setCallbacks(new MyCallbacks());
  pCharacteristicID->setValue(id);
  pCharacteristicID->setCallbacks(new MyCallbacks());
  pCharacteristicSWV->setValue("v0.1 - Oct 2020");
  pCharacteristicSWV->setCallbacks(new MyCallbacks());
  pCharacteristicAlert->setValue("None");
  pCharacteristicAlert->setCallbacks(new MyCallbacks());  
  pCharacteristicType->setValue("none");
  pCharacteristicType->setCallbacks(new MyCallbacks());
  pCharacteristicLat->setValue("0");
  pCharacteristicLat->setCallbacks(new MyCallbacks());
  pCharacteristicLong->setValue("0");
  pCharacteristicLong->setCallbacks(new MyCallbacks());
  pCharacteristicSSID->setValue(wifi_ssid);
  pCharacteristicSSID->setCallbacks(new MyCallbacks());
  pCharacteristicPASS->setValue(wifi_password);
  pCharacteristicPASS->setCallbacks(new MyCallbacks());
  pCharacteristicPASS2->setValue(wifi_password);
  pCharacteristicPASS->setCallbacks(new MyCallbacks());
  pService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  
  
  
  }




//--------------------------------------------------------------
//              Setup
//--------------------------------------------------------------
void setup() { 
   printf("Started IoT node");
   transmitingData= false;
   pinMode(LED_BUILTIN, OUTPUT);
   pinMode(ledPowerDust,OUTPUT);
   Serial.begin(115200);
   loadConfigNVS();
   if(strcmp(id, "0") == 0){}else{
//   configureWifi();
    //Wifi
   xTaskCreatePinnedToCore(
    keepWifiAlive,
    "Keep wifi alive",
    5000,
    NULL,
    1,
    NULL,
    CONFIG_ARDUINO_RUNNING_CORE
    );
    //Blink
    xTaskCreatePinnedToCore(
    blinkNoConnection,
    "Blink no connection",
    5000,
    NULL,
    1,
    NULL,
    CONFIG_ARDUINO_RUNNING_CORE
    );
    
    }
   initializeCharacteristics();  
   // Initialize SPIFFS
   if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    client.setServer("35.237.59.165", 1883);
}


//--------------------------------------------------------------
//              Verify connection with broker
//--------------------------------------------------------------
void verifyBrokerConnection(){
  
  if (!client.connected()) {
    transmitingData = false;
    if(WiFi.status()==WL_CONNECTED){
    Serial.println("Conectando con cliente");
    IPAddress myIP = WiFi.localIP();
   printf("Device IP: %s \n",myIP);
   Serial.println(myIP);
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
   
   char cstrPolvo[16];
   char cstrMQ135[16];

   String value = "AQ/Measurement/";
      char i[50];
      value.toCharArray(i, 50);
  
      char* info;
      info = (char*)malloc(50); 
      strcpy(info, i);
      strcat (info,id );
      char* info1;
      info1 = (char*)malloc(50);
      strcpy(info1, info);
      strcat (info1,"/CO" );
      char* info2;
      info2 = (char*)malloc(50);
      strcpy(info2, info);
      strcat (info2,"/CO2" );
      char* info3;
      info3 = (char*)malloc(50);
      strcpy(info3, info);
      strcat (info3,"/PM2.5" );
     sensorValue = analogRead(Gas_MQ7_analog);
  
      sensor_volt = sensorValue/1024*5.0;
      RS_gas = (5.0-sensor_volt)/sensor_volt;
      ratio = RS_gas/R0;
      float x = 1538.46 * ratio;
      float ppm = pow(x,-1.709);
      gcvt(ppm, 3, cstr);
      gcvt(dustDensity, 3, cstrPolvo);

      // MQ135
ppmMQ135 = gasSensor.getPPM();
      sensorValueMQ135 = analogRead(Gas_MQ135_analog);
//      sensorValueMQ135 = analogRead(Gas_MQ7_analog);
      sensor_voltMQ135 = sensorValueMQ135/1024*5.0;
//      float ppmMQ135 = pow(((R_L*(1/sensor_voltMQ135-1))/R0_MQ135)/a,1/b);
      gcvt(ppmMQ135/10000, 3, cstrMQ135);

    if(transmitingData){
      Serial.println("Sending to broker");
       client.publish(info1,cstr );
       client.publish(info2,cstrMQ135 );
       client.publish(info3,cstrPolvo );
    }
    else{
      storeData(cstr,cstrMQ135,cstrPolvo);
    }
      
  
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
//      writeWifiNVS(wifi_ssid, wifi_password);
    } else if (String(token)=="WIFI-PASS\r\n"){
      wifi_password= info;
//      writeWifiNVS(wifi_ssid, wifi_password);
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
//  digitalWrite(LED_BUILTIN, LOW);
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
      if(strcmp(id, "0") == 0){
        blinkConfig();
        }
        else{
          verifyBrokerConnection();
//     if (transmitingData){
      readSensors();
    
//      }
          }
     
     }
   client.loop();
}
