#ifndef INC_CLI_H_
#define INC_CLI_H_

#include <stdint.h>
#include "stm32f1xx.h"
#include "protocol.h"
#include "communication.h"
#include <string.h>

#define CLI_BUFFER_SIZE 64
typedef enum{
    CLI_CMD_INVALID,
    CLI_CMD_CLEAR,
    CLI_CMD_GOTO,
    CLI_CMD_PRINT,
    CLI_CMD_HELP
} CLI_CommandType;
typedef enum {
    CLI_STATE_IDLE,
    CLI_STATE_BUSY
} CLI_State_t;
void CLI_Init(void);
void CLI_ProcessByte(uint8_t byte);
void CLI_Process(void);

#endif /* INC_CLI_H_ */
