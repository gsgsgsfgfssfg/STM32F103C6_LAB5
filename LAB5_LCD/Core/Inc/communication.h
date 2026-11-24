#ifndef INC_COMMUNICATION_H_
#define INC_COMMUNICATION_H_

#include "protocol.h"
#include "stm32f1xx_hal.h"
#include "application.h"

HAL_StatusTypeDef Communication_Send(const Protocol_Frame_t *frame);
HAL_StatusTypeDef Communication_StartReceive(void);
uint8_t Communication_TryReceive(Protocol_Frame_t *frame);
void Communication_Process(void);
void Communication_TxCallback(void);
void Communication_RxCallback(void);
void Communication_ErrorCallback(void);

#endif /* INC_COMMUNICATION_H_ */
