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

/* =========================================================
 * Transmission
 * ========================================================= */
HAL_StatusTypeDef Communication_Send(const Protocol_Frame_t *frame)
{
    if (frame == NULL)
    {
        return HAL_ERROR;
    }

    if (tx_busy != 0U)
    {
        return HAL_BUSY;
    }

    if (Protocol_Pack(frame, tx_buffer) != PROTOCOL_OK)
    {
        return HAL_ERROR;
    }

    tx_busy = 1U;

    HAL_StatusTypeDef status =
        HAL_UART_Transmit_IT(&huart1,
                            tx_buffer,
                            PROTOCOL_FRAME_SIZE);

    if (status != HAL_OK)
    {
        tx_busy = 0U;
    }

    return status;
}

/* =========================================================
 * TX Complete Callback
 * ========================================================= */

void Communication_TxCallback(void)
{
    tx_busy = 0U;
}

/* =========================================================
 * Reception
 * ========================================================= */

HAL_StatusTypeDef Communication_StartReceive(void)
{
    if (rx_busy != 0U)
    {
        return HAL_BUSY;
    }

    rx_busy = 1U;

    HAL_StatusTypeDef status =
        HAL_UART_Receive_IT(&huart1,
                           &rx_byte,
                           1U);

    if (status != HAL_OK)
    {
        rx_busy = 0U;
    }

    return status;
}

/* =========================================================
 * RX Complete Callback
 * ========================================================= */

void Communication_RxCallback(void)
{
    if (rx_index == 0U)
    {
        if (rx_byte == PROTOCOL_START_BYTE)
        {
            rx_buffer[rx_index++] = rx_byte;
        }
    }
    else
    {
        rx_buffer[rx_index++] = rx_byte;

        if (rx_index == PROTOCOL_FRAME_SIZE)
        {
            Protocol_Frame_t frame;
            Protocol_InitFrameNull(&frame);

            if ((Protocol_Unpack(rx_buffer, &frame) == PROTOCOL_OK) &&
                (frame_ready == 0U))
            {
                received_frame = frame;
                frame_ready = 1U;
            }

            rx_index = 0U;
        }
    }

    rx_busy = 0U;
    Communication_StartReceive();
}

void Communication_ErrorCallback(void)
{
    rx_index = 0U;
    rx_busy = 0U;
    Communication_StartReceive();
}

uint8_t Communication_TryReceive(Protocol_Frame_t *frame)
{
    if ((frame == NULL) || (frame_ready == 0U))
    {
        return PROTOCOL_ERROR;
    }

    *frame = received_frame;
    frame_ready = 0U;
    return PROTOCOL_OK;
}

