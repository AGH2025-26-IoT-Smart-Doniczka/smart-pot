#pragma once

#include "esp_event.h"
#include "app_types.h"
#include <stdint.h>
#include <stdbool.h>


/* =========================================================================
   Event Base Declaration
   ========================================================================= */
ESP_EVENT_DECLARE_BASE(APP_EVENTS);

/* =========================================================================
   Events
   ========================================================================= */
typedef enum {
    // Initialization 
    APP_EVENT_CONFIG_LOADED = 0,    
    APP_EVENT_NO_CONFIG,           

    // Provisioning
    APP_EVENT_PROV_CONNECTED,       
    APP_EVENT_PROV_DATA_RECVD,      
    APP_EVENT_PROV_TIMEOUT,         
   APP_EVENT_REQUIRES_CALIBRATION,

    // Connectivity
    APP_EVENT_WIFI_CONNECTED,       
    APP_EVENT_WIFI_DISCONNECTED,    
    APP_EVENT_TIME_SYNC_DONE,       

    // Sensing
    APP_EVENT_SENSORS_DATA_READY, 
      APP_EVENT_CALIB_TIMEOUT,
    
    // Decision
    APP_EVENT_DECISION_MQTT,
    APP_EVENT_DECISION_STORAGE,

    // Data Handling
    APP_EVENT_MQTT_PUBLISHED,      
    APP_EVENT_STORAGE_SAVED,        

   // Idle
   APP_EVENT_IDLE_TIMEOUT,

    // Interrupts
    APP_EVENT_BTN1_SHORT,       // primary button  
    APP_EVENT_BTN1_3S,          // starts provisioning 
    APP_EVENT_BTN1_10S,         // factory reset
    APP_EVENT_BTN2_SHORT,       // secondary button
    APP_EVENT_BTN2_3S,          // --
    APP_EVENT_BTN2_10S,         // --
    APP_EVENT_FACTORY_RESET_DONE   
} app_event_id_t;

/* =========================================================================
   SECTION: Event Payload Types
   ========================================================================= */
typedef sensor_data_t sensor_event_data_t;


