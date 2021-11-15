#include "FS.h"
#include "SPIFFS.h"
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>

#define FORMAT_SPIFFS_IF_FAILED true

const char* ssid       = "Ana";
const char* password   = "nico1234";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -5*3600;
const int   daylightOffset_sec = 3600;
char fileCO[3000];
char fileCO2[30];
char filePM[30];
int i;

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

  
    
  strftime(fileCO,30, "/CO#%Y-%b-%d.txt", &timeinfo);
  strftime(fileCO2,30, "/CO2#%Y-%b-%d.txt", &timeinfo);
  strftime(filePM,30, "/PM#%Y-%b-%d.txt", &timeinfo);


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
void setup()
{
  Serial.begin(115200);
  
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  //disconnect WiFi as it's no longer needed
//  WiFi.disconnect(true);
//  WiFi.mode(WIFI_OFF);

   if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
        Serial.println("SPIFFS Mount Failed");
        return;
    }
    
    
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    Serial.println("Time variables");
char dateTime[3];
strftime(dateTime,3, "%H", &timeinfo);
Serial.println(dateTime);
char timeWeekDay[10];
strftime(timeWeekDay,10, "%A", &timeinfo);
Serial.println(timeWeekDay);
Serial.println();
//char fullDate[30];
//strftime(fullDate,30, "/%Y-%b-%dT%H:%M:%S.txt", &timeinfo);
//Serial.println(fullDate);
Serial.println();
//clearDir(SPIFFS, "/", 0);
configFiles();
 

  
    
//    writeFile(SPIFFS, fullDate, "Hello ");
//    appendFile(SPIFFS, fullDate, "World!\r\n");
    //readFile(SPIFFS, "/hello.txt");
    //renameFile(SPIFFS, "/hello.txt", "/foo.txt");
//    readFile(SPIFFS, fileCO2);
    //deleteFile(SPIFFS, "/foo.txt");
    //testFileIO(SPIFFS, "/test.txt");
    //deleteFile(SPIFFS, "/test.txt");
    Serial.println( "Test complete" );
    i=0;
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
 
  appendFile(SPIFFS, fileCO, lineCO);
  appendFile(SPIFFS, fileCO2, "World!\r\n");
  appendFile(SPIFFS, filePM, "World!\r\n");

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

  //-------------------------------------------

  
  //-------------------------------------------
  //-------------------------------------------
  
  //-------------------------------------------
  //-------------------------------------------
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
            
            
            int id = 23;
            String base = "http://192.168.0.4:3000/measurement?device="+(String)id +"&variable="+variable+"&timestamp="+day+"T";
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

void loop()
{
//  delay(100000);
  
//  storeData("2.2","5.3","7.5");
  if(i<1)
  {
    i=i+1;
uploadStoredData();
//clearDir(SPIFFS, "/", 0);
    
    
    }
  //printLocalTime();
}
