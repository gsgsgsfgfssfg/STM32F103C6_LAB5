#ifndef INC_COMMUNICATION_H_
#define INC_COMMUNICATION_H_

#include "protocol.h"
#include "stm32f1xx_hal.h"

#define COMM_ACK_TIMEOUT_MS    100
#define COMM_MAX_RETRIES      3

typedef enum
{
    COMM_RESULT_PENDING,
    COMM_RESULT_SUCCESS,
    COMM_RESULT_FAILED
} Communication_Result_t;

typedef enum
{
    COMM_STATE_IDLE,
    COMM_STATE_WAIT_TX,
    COMM_STATE_WAIT_ACK,
    COMM_STATE_RETRY,
    COMM_STATE_SUCCESS,
    COMM_STATE_FAILED
} Communication_State_t;

void Communication_Init(void);

HAL_StatusTypeDef Communication_Send(const Protocol_Frame_t *frame);
HAL_StatusTypeDef Communication_StartReceive(void);
uint8_t Communication_TryReceive(Protocol_Frame_t *frame);
void Communication_TxCallback(void);
void Communication_RxCallback(void);
void Communication_ErrorCallback(void);
HAL_StatusTypeDef Communication_Submit(const Protocol_Frame_t *frame);
void Communication_Process(void);
Communication_Result_t Communication_GetResult(void);

#endif /* INC_COMMUNICATION_H_ */
