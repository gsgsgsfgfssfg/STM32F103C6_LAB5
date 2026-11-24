#ifndef INC_APPLICATION_H_
#define INC_APPLICATION_H_

#include "protocol.h"
#include "lcd.h"

void Application_Init(void);
void Application_Process(void);
void Application_ProcessFrame(const Protocol_Frame_t *frame);

#endif /* INC_APPLICATION_H_ */
