#include "protocol.h"

/* =========================================================
 * Khởi tạo frame
 * ========================================================= */

void Protocol_InitFrameNull(Protocol_Frame_t *frame){
    if (frame == NULL) {
        return;
    }

    frame->start = PROTOCOL_START_BYTE;
    frame->cmd   = 0U;
    frame->seq   = 0U;
    frame->len   = 0U;

    for (uint8_t i = 0U; i < PROTOCOL_DATA_SIZE; i++) {
        frame->data[i] = 0U;
    }

    frame->crc = 0U;
    frame->end = PROTOCOL_END_BYTE;
}

void Protocol_InitFrame(
    Protocol_Frame_t *frame,
    uint8_t cmd,
    uint8_t seq,
    uint8_t len,
    const uint8_t *data
){
    if (frame == NULL) {
        return;
    }

    /* Không cho payload vượt kích thước vùng DATA 58 byte. */
    if (len > PROTOCOL_DATA_SIZE) {
        return;
    }

    frame->start = PROTOCOL_START_BYTE;
    frame->cmd   = cmd;
    frame->seq   = seq;
    frame->len   = len;
    frame->end   = PROTOCOL_END_BYTE;

    /* Xóa toàn bộ payload để byte thừa không mang dữ liệu cũ. */
    for (uint8_t i = 0U; i < PROTOCOL_DATA_SIZE; i++) {
        frame->data[i] = 0U;
    }

    /* Sao chép payload nếu có dữ liệu đầu vào. */
    if ((data != NULL) && (len > 0U)) {
        for (uint8_t i = 0U; i < len; i++) {
            frame->data[i] = data[i];
        }
    }

    Protocol_UpdateCRC(frame);
}

/* =========================================================
 * CRC-8
 * Polynomial: 0x07
 * Initial:    0x00
 * CRC dữ liệu: CMD + SEQ + LEN + DATA[58]
 * ========================================================= */

uint8_t Protocol_CalculateCRC(const uint8_t *data, uint16_t length){
    uint8_t crc = 0x00U;

    if (data == NULL) {
        return crc;
    }

    for (uint16_t i = 0U; i < length; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1U) ^ 0x07U);
            } else {
                crc = (uint8_t)(crc << 1U);
            }
        }
    }

    return crc;
}

/* Tính CRC trực tiếp từ các trường liên tục bắt đầu từ frame->cmd. */
static uint8_t Protocol_CalculateFrameCRC(const Protocol_Frame_t *frame){
    if (frame == NULL) {
        return 0U;
    }

    return Protocol_CalculateCRC((const uint8_t *)&frame->cmd,
                                 PROTOCOL_CRC_INPUT_SIZE);
}

void Protocol_UpdateCRC(Protocol_Frame_t *frame){
    if (frame == NULL) {
        return;
    }

    frame->crc = Protocol_CalculateFrameCRC(frame);
}

/* =========================================================
 * Kiểm tra độ dài payload theo từng loại lệnh
 * ========================================================= */

uint8_t Protocol_ValidateLength(const Protocol_Frame_t *frame){
    if (frame == NULL) {
        return PROTOCOL_ERROR;
    }

    if (frame->len > PROTOCOL_DATA_SIZE) {
        return PROTOCOL_ERROR;
    }

    switch (frame->cmd) {
        case CMD_CLEAR:
            return (frame->len == 0U) ? PROTOCOL_OK : PROTOCOL_ERROR;

        case CMD_GOTO:
            return (frame->len == 2U) ? PROTOCOL_OK : PROTOCOL_ERROR;

        case CMD_PRINT:
            /* PRINT phải có ít nhất 1 ký tự. */
            return (frame->len >= 1U) ? PROTOCOL_OK : PROTOCOL_ERROR;

        case CMD_ACK:
            return (frame->len == 2U) ? PROTOCOL_OK : PROTOCOL_ERROR;

        default:
            return PROTOCOL_ERROR;
    }
}

/* =========================================================
 * Kiểm tra toàn bộ frame
 * ========================================================= */

uint8_t Protocol_ValidateFrame(const Protocol_Frame_t *frame){
    if (frame == NULL) {
        return PROTOCOL_ERROR;
    }

    /* Kiểm tra byte đánh dấu đầu và cuối frame. */
    if (frame->start != PROTOCOL_START_BYTE) {
        return PROTOCOL_ERROR;
    }

    if (frame->end != PROTOCOL_END_BYTE) {
        return PROTOCOL_ERROR;
    }

    /* Kiểm tra mã lệnh và LEN tương ứng. */
    if (Protocol_ValidateLength(frame) != PROTOCOL_OK) {
        return PROTOCOL_ERROR;
    }

    /* So sánh CRC nhận được với CRC tính lại. */
    if (Protocol_CalculateFrameCRC(frame) != frame->crc) {
        return PROTOCOL_ERROR;
    }

    return PROTOCOL_OK;
}

/* =========================================================
 * Đóng gói frame
 * ========================================================= */

uint8_t Protocol_Pack(const Protocol_Frame_t *frame, uint8_t *buffer){
    if ((frame == NULL) || (buffer == NULL)) {
        return PROTOCOL_ERROR;
    }

    if (Protocol_ValidateLength(frame) != PROTOCOL_OK) {
        return PROTOCOL_ERROR;
    }

    /* START: byte 0. */
    buffer[PROTOCOL_START_OFFSET] = PROTOCOL_START_BYTE;

    /* Header: byte 1-3. */
    buffer[PROTOCOL_CMD_OFFSET] = frame->cmd;
    buffer[PROTOCOL_SEQ_OFFSET] = frame->seq;
    buffer[PROTOCOL_LEN_OFFSET] = frame->len;

    /* Payload: byte 4-61. */
    for (uint8_t i = 0U; i < PROTOCOL_DATA_SIZE; i++) {
        buffer[PROTOCOL_DATA_OFFSET + i] = frame->data[i];
    }

    /* CRC byte 62: bao phủ byte 1-61. */
    buffer[PROTOCOL_CRC_OFFSET] =
        Protocol_CalculateCRC(&buffer[PROTOCOL_CMD_OFFSET],
                              PROTOCOL_CRC_INPUT_SIZE);

    /* END: byte 63. */
    buffer[PROTOCOL_END_OFFSET] = PROTOCOL_END_BYTE;

    return PROTOCOL_OK;
}

/* =========================================================
 * Giải mã frame
 * ========================================================= */

uint8_t Protocol_Unpack(const uint8_t *buffer, Protocol_Frame_t *frame){
    if ((buffer == NULL) || (frame == NULL)) {
        return PROTOCOL_ERROR;
    }

    frame->start = buffer[PROTOCOL_START_OFFSET];
    frame->cmd   = buffer[PROTOCOL_CMD_OFFSET];
    frame->seq   = buffer[PROTOCOL_SEQ_OFFSET];
    frame->len   = buffer[PROTOCOL_LEN_OFFSET];

    for (uint8_t i = 0U; i < PROTOCOL_DATA_SIZE; i++) {
        frame->data[i] = buffer[PROTOCOL_DATA_OFFSET + i];
    }

    frame->crc = buffer[PROTOCOL_CRC_OFFSET];
    frame->end = buffer[PROTOCOL_END_OFFSET];

    /* Chỉ chấp nhận frame khi toàn bộ trường dữ liệu hợp lệ. */
    if (Protocol_ValidateFrame(frame) != PROTOCOL_OK) {
        return PROTOCOL_ERROR;
    }

    return PROTOCOL_OK;
}
