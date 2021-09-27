#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"


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


void setup() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
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


        //--------------------------------------------
        //                  ID
        //--------------------------------------------

        // Write
        writeStringNVS(my_handle, "id", "A123h");        

         // Read
        readStringNVS(my_handle,"id");


        //--------------------------------------------
        //                  wifi-ssid
        //--------------------------------------------

        // Write
        writeStringNVS(my_handle, "wifi-ssid", "Ana");        

         // Read
        readStringNVS(my_handle,"wifi-ssid");


        //--------------------------------------------
        //                  wifi-pass
        //--------------------------------------------

        // Write
        writeStringNVS(my_handle, "wifi-pass", "nicolashernandez1");        

         // Read
        readStringNVS(my_handle,"wifi-pass");


        //--------------------------------------------
        //                  ip-broker
        //--------------------------------------------

        // Write
        writeStringNVS(my_handle, "ip-broker", "192.168.1.103");        

         // Read
        readStringNVS(my_handle,"ip-broker");


        //--------------------------------------------
        //                  port
        //--------------------------------------------

        // Write
        writeIntNVS(my_handle, "port", 1883);        

         // Read
        readIntNVS(my_handle,"port");


        //--------------------------------------------
        //                  sample-time
        //--------------------------------------------

        // Write
        writeIntNVS(my_handle, "sample-time", 5000);        

         // Read
        readIntNVS(my_handle,"sample-time");

        //--------------------------------------------
        //                  mobile
        //--------------------------------------------

        // Write
        writeIntNVS(my_handle, "mobile", 1);        

         // Read
        readIntNVS(my_handle,"mobile");






        

        // Close
        nvs_close(my_handle);
    }

    printf("\n");

//    // Restart module
//    for (int i = 10; i >= 0; i--) {
//        printf("Restarting in %d seconds...\n", i);
//        vTaskDelay(1000 / portTICK_PERIOD_MS);
//    }
//    printf("Restarting now.\n");
//    fflush(stdout);
//    //esp_restart();
}

void loop() {
  // put your main code here, to run repeatedly:

}
