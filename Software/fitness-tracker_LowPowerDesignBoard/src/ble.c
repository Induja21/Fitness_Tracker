/***********************************************************************
* @file ble.c
* @version 0.0.1
* @brief This file has implementations related to all ble functionality
*
* @author Induja Narayanan, Induja.Narayanan@Colorado.edu
* @date Feb 15,2025
*
* @institution University of Colorado Boulder (UCB)
* @course ECEN 5823-001: IoT Embedded Firmware (Fall 2025)
* @instructor Chris Choi
*
* @assignment ecen5823-assignment2-IndujaNarayanan
* @due Feb 13,2025
*
* @resources Utilized Silicon Labs' EMLIB peripheral libraries to
* implement functionality.
* -
* -
*
* @copyright All rights reserved. Distribution allowed only for the
* use of assignment grading. Use of code excerpts allowed at the
* discretion of author. Contact for permission.
*/

#include "ble.h"
#include "sl_bt_api.h"
#include "scheduler.h"
#include "gatt_db.h"
#include "lcd.h"
#define INCLUDE_LOG_DEBUG 1
#include "log.h"
#include "gpio.h"
#include "timer.h"


// Set the advertising interval to 250 ms (min = max = 400 * 0.625 ms = 250 ms)
#define ADV_MIN_TIME 400
#define ADV_MAX_TIME 400

// Connection interval: 75 ms (60 * 1.25 ms = 75 ms)
#define CONN_INT_MIN 60
#define CONN_INT_MAX 60

// Slave latency: Allow the device to be "off the air" for up to 300 ms
// Slave latency = (desired off-air time) / (connection interval)
// Slave latency = 300 ms / 75 ms = 4
#define SLAVE_LATENCY 4

// Supervision timeout: Must be greater than (1 + slave latency) * (connection interval * 2)
// Supervision timeout > (1 + 4) * (75 ms * 2) = 5 * 150 ms = 750 ms
#define SUPERVISION_TIMEOUT (1+SLAVE_LATENCY)*(75*2)

// Global instance of the BLE data structure
static ble_data_struct_t ble_data = {
    .advertisingSetHandle = 0xFF, // Initialize to an invalid value
    .connection_open = false,
    .ok_to_send_htm_indications = false,
    .indication_in_flight = false
};
/* -------------------------------------------------------------------------------------
 * getBleData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function provides access to the global BLE data structure, which
 *            holds important Bluetooth-related information such as connection handles
 *            and advertising settings.
 * @Param   : None
 * @Return  : ble_data_struct_t* - Pointer to the global BLE data structure.
 *-------------------------------------------------------------------------------------*/
ble_data_struct_t* getBleData(void)
{
  return &ble_data;
}

/* -------------------------------------------------------------------------------------
 * start_advertising
 * ------------------------------------------------------------------------------------
 * @Purpose : This function initializes and starts BLE advertising. It creates an
 *            advertising set, generates advertising data from the GATT database,
 *            sets the advertising timing, and starts the advertising process.
 *            Errors during the process are logged for debugging.
 * @Param   : void
 * @Return  : void
 *-------------------------------------------------------------------------------------*/

static void start_advertising(void)
{
  sl_status_t status;

  // Create an advertising set
   status = sl_bt_advertiser_create_set(&ble_data.advertisingSetHandle);
   if (status != SL_STATUS_OK) {
       LOG_ERROR("Failed to create advertising set with error code %d", (int)status);
       return;
   }

   // Generate advertising data automatically from the GATT database
   status = sl_bt_legacy_advertiser_generate_data(ble_data.advertisingSetHandle, sl_bt_advertiser_general_discoverable);
   if (status != SL_STATUS_OK) {
       LOG_ERROR("Failed to generate advertising data with error code %d", (int)status);
       return;
   }

   // Set advertising timing (250 ms interval)
   status = sl_bt_advertiser_set_timing(
       ble_data.advertisingSetHandle,
       ADV_MIN_TIME, // Minimum advertising interval (400 * 0.625 ms = 250 ms)
       ADV_MAX_TIME, // Maximum advertising interval (400 * 0.625 ms = 250 ms)
       0,   // Advertising duration (0 means infinite)
       0    // Maximum number of advertising events (0 means infinite)
   );
   if (status != SL_STATUS_OK) {
       LOG_ERROR("Failed to set advertising timing with error code %d", (int)status);
       return;
   }

   // Start advertising
   status = sl_bt_legacy_advertiser_start(ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
   if (status != SL_STATUS_OK) {
       LOG_ERROR("Advertiser start failed with error code %d", (int)status);
   }
}

/* -------------------------------------------------------------------------------------
 * connection_established_event
 * ------------------------------------------------------------------------------------
 * @Purpose : This function handles the event when a BLE connection is established.
 *            It stores the connection handle, updates the connection status, stops
 *            advertising, and sets the connection parameters (interval, latency,
 *            and supervision timeout). Errors during the process are logged for debugging.
 * @Param   : sl_bt_msg_t *event - Pointer to the BLE event data containing connection details.
 * @Return  : void
 *-------------------------------------------------------------------------------------*/

void connection_established_event(sl_bt_msg_t *event)
{
    sl_status_t status;
    // Store the connection handle
    ble_data.connection_handle = event->data.evt_connection_opened.connection;

    // Update connection status
    ble_data.connection_open = true;
    displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
    // Stop advertising
    status = sl_bt_advertiser_stop(ble_data.advertisingSetHandle);
    if (status != SL_STATUS_OK) {
        LOG_ERROR("Failed to stop advertising with error code %d", (int)status);
    }
    //            LOG_INFO("Connection Interval set:%d",conn_min);
    //            LOG_INFO("Connection max Interval set:%d",conn_max);
    //            LOG_INFO("Slave latency set:%d",slave_latency);
    //            LOG_INFO("Supervision timeout:%d",supervisiontimeout);
    // Set connection parameters
    status = sl_bt_connection_set_parameters(
        ble_data.connection_handle,
        CONN_INT_MIN,
        CONN_INT_MAX,
        SLAVE_LATENCY,
        SUPERVISION_TIMEOUT, // SUPERVISION_TIMEOUT (5 seconds)
        0x00,
        0xffff
    );
    if (status != SL_STATUS_OK) {
        LOG_ERROR("Failed to set connection parameters with error code %d", (int)status);
    }
}
/* -------------------------------------------------------------------------------------
 * handle_ble_event
 * ------------------------------------------------------------------------------------
 * @Purpose : This function handles Bluetooth events triggered by the stack, such as
 *            system boot, connection opened/closed, and GATT server characteristic
 *            status changes. It manages advertising, connection parameters, and
 *            indications for the temperature measurement characteristic.
 * @Param   : sl_bt_msg_t *event - Pointer to the Bluetooth event structure.
 * @Return  : None
 *-------------------------------------------------------------------------------------*/
void handle_ble_event(sl_bt_msg_t *event) {
    sl_status_t status;
    bd_addr address;
    uint8_t address_type;

    switch (SL_BT_MSG_ID(event->header)) {
        case sl_bt_evt_system_boot_id: {
            //Initialize display
            displayInit();
            //Print Display contents
            displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
            displayPrintf(DISPLAY_ROW_NAME, "Server");
            // Get the Bluetooth identity address
            status = sl_bt_system_get_identity_address(&address, &address_type);

            if (status == SL_STATUS_OK) {
                // Print the address
                displayPrintf(DISPLAY_ROW_BTADDR,"%02X:%02X:%02X:%02X:%02X:%02X\n",
                        address.addr[0],
                       address.addr[1],
                       address.addr[2],
                       address.addr[3],
                       address.addr[4],
                       address.addr[5]);
            } else {
                LOG_ERROR("Failed to get Bluetooth address, error code: 0x%04x\n", (int)status);
            }
            //Start advertising
            start_advertising();
           // gpioInit();

            break;
        }

        case sl_bt_evt_connection_opened_id: {
            connection_established_event(event);
            break;
        }

        case sl_bt_evt_connection_closed_id: {
            // Update connection status
            ble_data.connection_open = false;
            ble_data.ok_to_send_htm_indications = false; // Disable indications when connection is closed
            displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");

            // Restart advertising when the connection is closed
            status = sl_bt_legacy_advertiser_start(ble_data.advertisingSetHandle, sl_bt_legacy_advertiser_connectable);
            if (status != SL_STATUS_OK) {
                LOG_ERROR("Failed to restart advertising with error code %d", (int)status);
            }
            displayPrintf(DISPLAY_ROW_TEMPVALUE,"");
            break;
        }
        case sl_bt_evt_connection_parameters_id:
        break;

        case sl_bt_evt_gatt_server_characteristic_status_id:
          {
            // Get the status flags and client configuration flags from the event
            uint8_t status_flags = event->data.evt_gatt_server_characteristic_status.status_flags;
            uint16_t client_config_flags = event->data.evt_gatt_server_characteristic_status.client_config_flags;
            uint16_t characteristic = event->data.evt_gatt_server_characteristic_status.characteristic;

            // Check if this is a client characteristic configuration change
            if ((status_flags == sl_bt_gatt_server_client_config)&&(characteristic == gattdb_temperature_measurement))
            {
                if (client_config_flags & sl_bt_gatt_indication)
                {
                  //Set a flag here to allow sending indications
                  ble_data.ok_to_send_htm_indications = true;

                }
                else
                {
                  // Clear the flag here to stop sending indications
                  ble_data.ok_to_send_htm_indications = false;
                  displayPrintf(DISPLAY_ROW_TEMPVALUE,"");
                }
            }
            // Check if this is an indication confirmation
            else if (status_flags == sl_bt_gatt_server_confirmation)
            {
              if (characteristic == gattdb_temperature_measurement)
              {
                // Indication for temperature measurement was confirmed
                // Clear a flag here to allow sending the next indication
                ble_data.indication_in_flight = false;
              }
            }
            break;
          }
        case sl_bt_evt_gatt_server_indication_timeout_id:
          ble_data.indication_in_flight = false;
          break;
        default:
            break;
        }

}

