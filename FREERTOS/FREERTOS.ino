/* 1º Ejemplo básico con tareas.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
int i;
#define TAM_COLA 20 /*20 mensajes*/
#define TAM_MSG 7 /*Cada Mensaje 7 caracteres*/
QueueHandle_t queue;
void setup(){
  i=0;
  nvs_flash_init();
  queue = xQueueCreate( TAM_COLA, TAM_MSG );
 if(queue == NULL){
    printf("Error creating the queue");
  }
//    xTaskCreate(&tarea1, "tarea1", 1024, NULL, 1, NULL);
//    xTaskCreate(&tarea2, "tarea2", 1024, NULL, 2, NULL);
//    xTaskCreate(&tarea3, "tarea3", 1024, NULL, 3, NULL);
//    xTaskCreate(&tarea4, "tarea4", 1024, NULL, 4, NULL);
     xTaskCreate(&lee1,     "lee1",     1024, NULL, 5, NULL);
    xTaskCreate(&escribe1, "escribe1", 1024, NULL, 1, NULL);
    
    xTaskCreate(&escribe2, "escribe2", 1024, NULL, 1, NULL);
    xTaskCreate(&escribe3, "escribe3", 1024, NULL, 1, NULL);

  }

void loop(){
  }
void tarea1(void *pvParameter)
{
    while(1) {
      printf("Ejecutando tarea 1\n");
//      printf("%d",i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    vTaskDelete(NULL);
}

void tarea2(void *pvParameter)
{
    while(1) {
       printf("Ejecutando tarea 2\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    vTaskDelete(NULL);
}

void tarea3(void *pvParameter)
{
    while(1) {
      printf("Ejecutando tarea 3\n");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
      i=i+1;
    }
    vTaskDelete(NULL);
}

void tarea4(void *pvParameter)
{
    while(1) {
       printf("Ejecutando tarea 4\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    vTaskDelete(NULL);
}






void lee1(void *pvParameter)
{
   int i;
   char Rx[7];
   while(1) {

    if(xQueueReceive(queue,&Rx,10000/portTICK_RATE_MS)==pdTRUE) {//10s --> Tiempo max. que la tarea está bloqueada si la cola está vacía

         for(i=0; i<strlen(Rx); i++)
           {
             printf("%c",Rx[i]);
           }

          } else{
             printf("Fallo al leer la cola");

     }

   }
}

void escribe1(void *pvParameter)
{
    char cadena[7];
    while(1) {
    strcpy (cadena, "Tarea1\n");
      if (xQueueSendToBack(queue, &cadena,2000/portTICK_RATE_MS)!=pdTRUE){//2seg--> Tiempo max. que la tarea está bloqueada si la cola está llena
         printf("error escribe1\n");

    }
      vTaskDelay(2000 / portTICK_RATE_MS);
 }
}


void escribe2(void *pvParameter)
{
    char cadena[7];
    while(1) {

       strcpy (cadena, "Tarea2\n");
       if (xQueueSendToBack(queue, &cadena,2000/portTICK_RATE_MS)!=pdTRUE){
          printf("error escribe2\n");
    }
       vTaskDelay(2000 / portTICK_RATE_MS);
 }

}

void escribe3(void *pvParameter)
{
    char cadena[7];
    while(1) {
       strcpy (cadena, "Tarea3\n");
       if (xQueueSendToBack(queue, &cadena,2000/portTICK_RATE_MS)!=pdTRUE){
          printf("error escribe3\n");
          }
       vTaskDelay(2000 / portTICK_RATE_MS);
       }
    }
