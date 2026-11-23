#ifndef INC_CLI_H_
#define INC_CLI_H_

#include <stdint.h>
#include "stm32f1xx.h"
#include "lcd.h"
#include <string.h>

#define CLI_BUFFER_SIZE 64

void CLI_Init(void);
void CLI_ProcessByte(uint8_t byte);
void CLI_Process(void);

#endif /* INC_CLI_H_ */
