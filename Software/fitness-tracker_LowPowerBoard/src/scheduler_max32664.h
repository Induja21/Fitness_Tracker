/*
 * scheduler_max32664.h
 *
 *  Created on: 08-Nov-2025
 *      Author: nindu
 */

#ifndef SRC_SCHEDULER_MAX32664_H_
#define SRC_SCHEDULER_MAX32664_H_

#include "sl_bt_api.h"


typedef enum max32664InitState
{
    MAX32664_INIT_SUCCESSFUL,
    MAX32664_INIT_IDLE,
    MAX32664_INIT_IN_PROGRESS,
    MAX32664_INIT_FAILED
} max32664InitState_e;
void max32664StateMachine(sl_bt_msg_t *bleEvent);
max32664InitState_e getLatestInitState(void);
void max32664StateMachineCalibrationMode(sl_bt_msg_t *bleEvent);

#endif /* SRC_SCHEDULER_MAX32664_H_ */
