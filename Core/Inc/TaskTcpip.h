//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

#include "cmsis_os.h"
#include "queue.h"

// ***********************************************************
// 各種定義
// ***********************************************************
#define HDLC_HEADER_SIZE 3 // control, count, len
#define Q_DATA_SIZE 80
#define FSC_SIZE 2

#define HDLC_ADDR_HK 0b00
#define HDLC_ADDR_CMD 0b01
#define HDLC_ADDR_IMG 0b10
//
#define HDLC_POS_HEAD 0b00
#define HDLC_POS_BODY 0b01
#define HDLC_POS_TAIL 0b10
#define HDLC_POS_SINGLE 0b11

typedef struct {
    uint8_t addr: 4; // 宛先アドレス
    uint8_t pos: 2;  // HEAD 0b00, ... SINGLE 0b11
    uint8_t isAbort: 1; // 亜ボードパケット
    uint8_t dummy: 1; //
} HDLCControlType;

typedef struct {
    HDLCControlType control;
    uint8_t count;
    uint8_t len;
    uint8_t data[Q_DATA_SIZE];
} xQueuePacketType;
_Static_assert(sizeof(xQueuePacketType) == HDLC_HEADER_SIZE + Q_DATA_SIZE, "HDLC_HEADER_SIZE mismatch");

extern QueueHandle_t xQueueTx;
extern QueueHandle_t xQueueRxHK;
extern QueueHandle_t xQueueRxCMD;
extern QueueHandle_t xQueueRxIMG;
extern int Is_gse_mode;

// ***********************************************************
// 関数
// ***********************************************************
void TaskCMX869BEntry(void *argument);
void INT_CMX869B_ISR(void);
void CMX869B_putc(uint8_t tx_data);
void CMX869B_get_qam_status(uint8_t *mode, uint8_t *snr, uint8_t *msg);

void uart_putc(unsigned char c);

#endif //CMX869B_TASKCMX869B_H
