#include "application.h"

/* =========================================================
 * Private Variables
 * ========================================================= */

/* TODO: Declare application-specific variables */
static uint8_t application_status;

/* =========================================================
 * Initialization
 * ========================================================= */

void Application_Init(void)
{
    /* TODO: Initialize application state */
	application_status = ACK_STATUS_OK;
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
    if (frame == NULL || Protocol_ValidateFrame(frame) == 0) return;
    uint8_t status = ACK_STATUS_OK;

    switch (frame->cmd){
        case CMD_CLEAR:
            if (frame->len != 0U) status = ACK_STATUS_INVALID_LEN;
            else{
                lcd_clear();
            }
            break;

        case CMD_GOTO:
            if (frame->len != 2U){
                status = ACK_STATUS_INVALID_LEN;
            }
            else lcd_goto(frame->data[0], frame->data[1]);
            break;

        case CMD_PRINT:
            if (frame->len > PROTOCOL_DATA_SIZE) {
                status = ACK_STATUS_INVALID_LEN;
            }
            else {
                char print_buffer[PROTOCOL_DATA_SIZE + 1U];

                for (uint8_t i = 0U; i < frame->len; i++){
                    print_buffer[i] = (char)frame->data[i];
                }

                print_buffer[frame->len] = '\0';
                lcd_print(print_buffer);
            }
            break;

        default:
            status = ACK_STATUS_INVALID_CMD;
            break;
    }

    /* =====================================================
     * ACK Response
     * ===================================================== */

    Protocol_Frame_t ack_frame;

    uint8_t ack_data[2] ={frame->cmd,status};

    Protocol_InitFrame(&ack_frame,CMD_ACK,frame->seq,2U,ack_data);
    Communication_Send(&ack_frame);
}
