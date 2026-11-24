#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "stm32f1xx.h"

/* =========================================================
 * Cấu hình frame
 * ========================================================= */

#define PROTOCOL_FRAME_SIZE       64U
#define PROTOCOL_START_BYTE       0x21U /* '!' */
#define PROTOCOL_END_BYTE         0x23U /* '#' */

/* Vị trí byte trong buffer truyền UART. */
#define PROTOCOL_START_OFFSET     0U
#define PROTOCOL_CMD_OFFSET       1U
#define PROTOCOL_SEQ_OFFSET       2U
#define PROTOCOL_LEN_OFFSET       3U
#define PROTOCOL_DATA_OFFSET      4U
#define PROTOCOL_DATA_SIZE        58U
#define PROTOCOL_CRC_OFFSET       62U
#define PROTOCOL_END_OFFSET       63U

/* Các byte tạo CRC: CMD + SEQ + LEN + DATA[58]. */
#define PROTOCOL_CRC_INPUT_SIZE   (3U + PROTOCOL_DATA_SIZE)

/* =========================================================
 * Command IDs
 * ========================================================= */

#define CMD_CLEAR                 0x01U
#define CMD_GOTO                  0x02U
#define CMD_PRINT                 0x03U
#define CMD_ACK                   0x04U

/* =========================================================
 * ACK status codes
 * ========================================================= */

#define ACK_STATUS_OK             0x00U
#define ACK_STATUS_INVALID_CMD    0x01U
#define ACK_STATUS_INVALID_LEN    0x02U
#define ACK_STATUS_CRC_ERROR      0x03U
#define ACK_STATUS_INVALID_PARAM  0x04U
#define ACK_STATUS_BUSY           0x05U

/** Trạng thái thành công/thất bại dùng cho các hàm đóng gói. */
#define PROTOCOL_OK               1U
#define PROTOCOL_ERROR            0U

/* =========================================================
 * Protocol frame
 * Layout 64 byte: ! [CMD][SEQ][LEN][DATA58][CRC][#]
 * ========================================================= */

typedef struct
{
    uint8_t start;                       /**< Byte bắt đầu = '!'. */
    uint8_t cmd;                         /**< Mã lệnh. */
    uint8_t seq;                         /**< Số thứ tự gói tin. */
    uint8_t len;                         /**< Số byte dữ liệu hợp lệ. */
    uint8_t data[PROTOCOL_DATA_SIZE];    /**< Vùng payload 58 byte. */
    uint8_t crc;                         /**< CRC-8 polynomial 0x07. */
    uint8_t end;                         /**< Byte kết thúc = '#'. */
} Protocol_Frame_t;

/* =========================================================
 * Protocol functions
 * ========================================================= */

/** Khởi tạo frame rỗng bằng các giá trị mặc định. */
void Protocol_InitFrameNull(Protocol_Frame_t *frame);

/**
 * Khởi tạo frame đầy đủ và tự cập nhật CRC.
 * Hàm không làm gì nếu frame == NULL hoặc len > 58 byte.
 */
void Protocol_InitFrame(
    Protocol_Frame_t *frame,
    uint8_t cmd,
    uint8_t seq,
    uint8_t len,
    const uint8_t *data
);

/** Tính CRC-8 (poly 0x07, initial 0x00). */
uint8_t Protocol_CalculateCRC(const uint8_t *data, uint16_t length);

/** Cập nhật CRC của frame trong bộ nhớ. */
void Protocol_UpdateCRC(Protocol_Frame_t *frame);

/** Kiểm tra byte đầu/cuối, lệnh, độ dài payload và CRC. */
uint8_t Protocol_ValidateFrame(const Protocol_Frame_t *frame);

/** Kiểm tra LEN có phù hợp với từng loại lệnh hay không. */
uint8_t Protocol_ValidateLength(const Protocol_Frame_t *frame);

/** Đóng gói frame thành buffer 64 byte. */
uint8_t Protocol_Pack(const Protocol_Frame_t *frame, uint8_t *buffer);

/** Giải mã buffer 64 byte và kiểm tra tính hợp lệ. */
uint8_t Protocol_Unpack(const uint8_t *buffer, Protocol_Frame_t *frame);

#endif /* PROTOCOL_H */
