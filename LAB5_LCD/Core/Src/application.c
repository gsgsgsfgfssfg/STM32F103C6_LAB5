#include "application.h"
#include "communication.h"

/* =========================================================
 * Private Variables
 * ========================================================= */

/* TODO: Declare application-specific variables */


/* =========================================================
 * Initialization
 * ========================================================= */

void Application_Init(void)
{
    /* TODO: Initialize application state */

    /* TODO: Initialize application modules if necessary */
}


/* =========================================================
 * Main Application Processing
 * ========================================================= */

void Application_Process(void)
{
    /* TODO: Process application tasks */

    /* TODO: Handle pending events */

    /* TODO: Update application state */
}


/* =========================================================
 * Frame Processing
 * ========================================================= */

void Application_ProcessFrame(const Protocol_Frame_t *frame){
    if (frame == NULL)  return;

    uint8_t command_executed = 1U;
    switch (frame->cmd)
    {
        case CMD_CLEAR:
        	lcd_clear();
            break;

        case CMD_GOTO:
        	lcd_goto(frame->data[0],frame->data[1]);
            break;

        case CMD_PRINT:{
        	char s[frame->len+1];
        	for(int i=0;i<frame->len;i++){
        		s[i]=(char) frame->data[i];
        	}
        	s[frame->len]='\0';
        	lcd_print(s);
            break;
        }
        default:
                command_executed = 0U;
            break;
    }

        if (command_executed != 0U)
        {
            Protocol_Frame_t ack_frame;
            uint8_t ack_data[2] = {frame->cmd, ACK_STATUS_OK};

            Protocol_InitFrame(&ack_frame, CMD_ACK, frame->seq, 2U, ack_data);
            (void)Communication_Send(&ack_frame);
        }
}
