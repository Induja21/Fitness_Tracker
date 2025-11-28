/*
 * scheduler_bmi270.h
 *
 *  Created on: 10-Nov-2025
 *      Author: nindu
 */

#ifndef SRC_SCHEDULER_BMI270_H_
#define SRC_SCHEDULER_BMI270_H_


#include "sl_bt_api.h"

typedef enum bmi270InitState
{
    BMI270_INIT_SUCCESSFUL,
    BMI270_INIT_IDLE,
    BMI270_INIT_IN_PROGRESS,
    BMI270_INIT_FAILED
} bmi270InitStatus_e;

void bmi270StateMachine(sl_bt_msg_t *bleEvent);
bmi270InitStatus_e getLatestBmi270InitState(void);
void bmi270DataHandlingStateMachine(sl_bt_msg_t *bleEvent);

#endif /* SRC_SCHEDULER_BMI270_H_ */
