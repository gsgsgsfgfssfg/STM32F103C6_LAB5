#include "communication.h"

extern UART_HandleTypeDef huart1;

/* =========================================================
 * Private variables
 * ========================================================= */

/* TX */
static uint8_t tx_buffer[PROTOCOL_FRAME_SIZE];
static volatile uint8_t tx_busy;

/* RX */
static uint8_t rx_buffer[PROTOCOL_FRAME_SIZE];
static uint8_t rx_byte;
static uint8_t rx_index;
static volatile uint8_t rx_busy;

/* Application delivery */
static Protocol_Frame_t received_frame;
static volatile uint8_t frame_ready;

/* Transaction */
static Communication_State_t comm_state;
static Protocol_Frame_t pending_frame;

static uint8_t next_seq;
static uint8_t retry_count;

/* TX completion event */
static volatile uint8_t tx_complete;

/* ACK timeout */
static uint32_t ack_timer;

/* Transaction result */
static Communication_Result_t comm_result;

void Communication_Init(void)
{
    comm_state = COMM_STATE_IDLE;

    next_seq = 0;
    retry_count = 0;

    comm_result = COMM_RESULT_PENDING;

    tx_complete = 0;
    ack_timer = 0;

    tx_busy = 0;
    rx_busy = 0;
    rx_index = 0;
    frame_ready = 0;

    Communication_StartReceive();
}

/* =========================================================
 * Transmission
 * ========================================================= */
HAL_StatusTypeDef Communication_Send(const Protocol_Frame_t *frame)
{
    if (frame == NULL) return HAL_ERROR;
    if (tx_busy != 0) return HAL_BUSY;
    if (Protocol_Pack(frame, tx_buffer) != PROTOCOL_OK) return HAL_ERROR;

    tx_busy = 1;
    HAL_StatusTypeDef status = HAL_UART_Transmit_IT(&huart1, tx_buffer, PROTOCOL_FRAME_SIZE);

    if (status != HAL_OK) tx_busy = 0;
    return status;
}

/* =========================================================
 * TX Complete Callback
 * ========================================================= */

void Communication_TxCallback(void){
    tx_busy = 0;
    tx_complete = 1;
}

/* =========================================================
 * Reception
 * ========================================================= */

HAL_StatusTypeDef Communication_StartReceive(void)
{
    if (rx_busy != 0) return HAL_BUSY;

    rx_busy = 1;
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    if (status != HAL_OK) rx_busy = 0;
    return status;
}

/* =========================================================
 * RX Complete Callback
 * ========================================================= */

void Communication_RxCallback(void)
{
    if (rx_index == 0) {
        if (rx_byte == PROTOCOL_START_BYTE) {
            rx_buffer[rx_index++] = rx_byte;
        }
    }
    else {
        rx_buffer[rx_index++] = rx_byte;

        if (rx_index == PROTOCOL_FRAME_SIZE){
            Protocol_Frame_t frame;
            Protocol_InitFrameNull(&frame);

            if ((Protocol_Unpack(rx_buffer, &frame) == PROTOCOL_OK) && (frame_ready == 0)){
                received_frame = frame;
                frame_ready = 1;
            }

            rx_index = 0;
        }
    }

    rx_busy = 0;
    Communication_StartReceive();
}

void Communication_ErrorCallback(void)
{
    rx_index = 0;
    rx_busy = 0;
    Communication_StartReceive();
}

uint8_t Communication_TryReceive(Protocol_Frame_t *frame)
{
    if ((frame == NULL) || (frame_ready == 0))
    {
        return PROTOCOL_ERROR;
    }

    *frame = received_frame;
    frame_ready = 0;
    return PROTOCOL_OK;
}

/* =========================================================
 * Stop-and-Wait ARQ
 * ========================================================= */

HAL_StatusTypeDef Communication_Submit(const Protocol_Frame_t *frame) {
    if (Protocol_ValidateFrame(frame) != PROTOCOL_OK) return HAL_ERROR;

    if (comm_state != COMM_STATE_IDLE) return HAL_BUSY;

    pending_frame = *frame;
    pending_frame.seq = next_seq;

    retry_count = 0;
    tx_complete = 0;
    comm_result = COMM_RESULT_PENDING;

    comm_state = COMM_STATE_WAIT_TX;

    HAL_StatusTypeDef status = Communication_Send(&pending_frame);

    if (status != HAL_OK){
        comm_state = COMM_STATE_FAILED;
        comm_result = COMM_RESULT_FAILED;
        return status;
    }

    return HAL_OK;
}

static uint8_t Communication_ValidateAck(const Protocol_Frame_t *ack, uint8_t *remote_status){
    if (ack == NULL || remote_status == NULL)
        return ACK_STATUS_INVALID_PARAM;

    if (ack->cmd != CMD_ACK)
        return ACK_STATUS_INVALID_CMD;

    if (ack->seq != pending_frame.seq)
        return ACK_STATUS_INVALID_PARAM;

    if (ack->len != 2)
        return ACK_STATUS_INVALID_LEN;

    if (ack->data[0] != pending_frame.cmd)
        return ACK_STATUS_INVALID_PARAM;

    *remote_status = ack->data[1];

    /* ACK matches this transaction */
    return ACK_STATUS_OK;
}

void Communication_Process(void){
    Protocol_Frame_t rx_frame;
    uint8_t remote_status;

    /* =====================================================
     * RX Processing
     * ===================================================== */

    if (Communication_TryReceive(&rx_frame) == PROTOCOL_OK){
    	//Communication_Send(&rx_frame);
        /* ACK is only meaningful during an active transaction */
        if ((comm_state == COMM_STATE_WAIT_TX) ||
            (comm_state == COMM_STATE_WAIT_ACK)) {
            /* Validate received frame */
            if ((Protocol_ValidateFrame(&rx_frame) == PROTOCOL_OK) ||
            		Communication_ValidateAck(&rx_frame,&remote_status) == ACK_STATUS_OK){
				/* ACK received: transaction was acknowledged */
				next_seq = (next_seq + 1) % 2;

				if (remote_status == ACK_STATUS_OK) comm_result = COMM_RESULT_SUCCESS;
				else comm_result = COMM_RESULT_FAILED;

				comm_state = (comm_result == COMM_RESULT_SUCCESS) ? COMM_STATE_SUCCESS : COMM_STATE_FAILED;
            }
        }
    }

    /* =====================================================
     * Transaction State Machine
     * ===================================================== */

    switch (comm_state) {
        case COMM_STATE_IDLE:

            /* No active transaction */
            break;


        case COMM_STATE_WAIT_TX:
            /* Wait for TX completion event */
            if (tx_complete != 0){
                tx_complete = 0;
                /* Start ACK timeout timer */
                ack_timer = HAL_GetTick();
                /* TX completed, now wait for ACK */
                comm_state = COMM_STATE_WAIT_ACK;
            }
            break;


        case COMM_STATE_WAIT_ACK:
            /* Check ACK timeout */
            if ((HAL_GetTick() - ack_timer) >= COMM_ACK_TIMEOUT_MS){
                /* Retry if retry limit has not been reached */
                if (retry_count < COMM_MAX_RETRIES) {
                    retry_count++;
                    comm_state = COMM_STATE_RETRY;
                }
                else{
                    /* Retry limit reached */
                    comm_result = COMM_RESULT_FAILED;
                    comm_state = COMM_STATE_FAILED;
                }
            }
            break;


        case COMM_STATE_RETRY:
            /* Clear old TX completion event */
            tx_complete = 0;
            /* Return to WAIT_TX before starting TX */
            comm_state = COMM_STATE_WAIT_TX;
            /* Retransmit the same frame */
            if (Communication_Send(&pending_frame) != HAL_OK){
                /* TX could not be started.
                 * Retry state will be handled again.
                 */
                comm_state = COMM_STATE_RETRY;
            }

            break;


        case COMM_STATE_SUCCESS:
        case COMM_STATE_FAILED:
            /* Preserve result until CLI retrieves it */
            break;

        default:

            /* Recover from unexpected state */
            comm_state = COMM_STATE_IDLE;
            comm_result = COMM_RESULT_FAILED;
            break;
    }
}

Communication_Result_t Communication_GetResult(void){
    Communication_Result_t result = comm_result;

    /* Transaction is still running */
    if (result == COMM_RESULT_PENDING){
        return COMM_RESULT_PENDING;
    }

    /* Release completed transaction */
    comm_state = COMM_STATE_IDLE;
    comm_result = COMM_RESULT_PENDING;

    return result;
}
