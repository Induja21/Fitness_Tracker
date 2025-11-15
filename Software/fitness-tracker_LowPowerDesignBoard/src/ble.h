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

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "sl_bgapi.h"
#include "sl_bt_api.h"
#include <stdint.h>

#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); }
#define UINT32_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
*(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }
#define INT32_TO_FLOAT(m, e) ( (int32_t) (((uint32_t) m) & 0x00FFFFFFU) | (((uint32_t) e) << 24) )
// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition
// Define the BLE data structure
typedef struct {
    // Values common to servers and clients
    bd_addr myAddress;
    uint8_t myAddressType;
    uint8_t connection_handle;
    // Values unique for server
    uint8_t advertisingSetHandle;
    bool connection_open; // True when in an open connection
    bool ok_to_send_htm_indications; // True when client enabled indications
    bool indication_in_flight; // True when an indication is in-flight

    // Values unique for client (not used in this example)
} ble_data_struct_t;
/* -------------------------------------------------------------------------------------
 * getBleData
 * ------------------------------------------------------------------------------------
 * @Purpose : This function provides access to the global BLE data structure, which
 *            holds important Bluetooth-related information such as connection handles
 *            and advertising settings.
 * @Param   : None
 * @Return  : ble_data_struct_t* - Pointer to the global BLE data structure.
 *-------------------------------------------------------------------------------------*/
ble_data_struct_t* getBleData(void);

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
void handle_ble_event(sl_bt_msg_t *event);
#endif /* SRC_BLE_H_ */
