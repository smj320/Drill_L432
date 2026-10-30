//
// Created by kikuchi on 2026/10/21.
//

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "TaskHK.h"
#include "TaskHDLC.h"
//
extern QueueHandle_t hTxQ, hRxQ;
extern osThreadId_t TaskHDLCHandle;
extern UART_HandleTypeDef huart2;

// printfはnewlibのstdoutロック(taskENTER_CRITICAL)を握ったままUART送信するので、
// 送信中はTIM2割込とタスク切替が止まり、HDLCが取りこぼす。
// 文字列を作ってからロックなしでUART送信する
static void hk_print(const char *fmt, ...) {
    static char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (len > (int) sizeof(buf) - 1) len = sizeof(buf) - 1;
    if (len > 0) HAL_UART_Transmit(&huart2, (uint8_t *) buf, len, HAL_MAX_DELAY);
}

// タスクのエントリポイント。weakをここで上書き
void StartTaskHK(void *argument) {
    uint8_t tx_data[Q_DATA_SIZE];
    uint8_t rx_data[Q_DATA_SIZE];
    static uint8_t f_count = 0;

    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        // 処理開始
        for (int i = 0; i < Q_DATA_SIZE; i++) {
            tx_data[i] = i;
        }
        // 送信実行
        xQueueSend(hTxQ, tx_data, 0);
        osDelay(200);

        // 受信
        if (xQueueReceive(hRxQ, rx_data, 1500) == pdTRUE) {
            hk_print("GSE %d : \r\n", Is_gse);
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                hk_print("%04d ", rx_data[i]);
            }
            hk_print("\r\n");
        } else {
            hk_print("Receive timeout\r\n");
        }
        // 受信状態
        hk_print("RDY:%lu OVF:%lu UDF:%lu FER:%lu FCS:%lu LEN:%d\r\n",
                 HdlcCnt.rxReady, HdlcCnt.rxOverflow, HdlcCnt.txUnderflow,
                 HdlcCnt.frameError, HdlcCnt.fcs, HdlcCnt.lastLen);
        // 送信状態とスタック残量(word単位。0に近いと溢れかけ)
        hk_print("SRV:%lu TXF:%lu STK HDLC:%lu HK:%lu\r\n",
                 HdlcCnt.service, HdlcCnt.txFrame,
                 (uint32_t) uxTaskGetStackHighWaterMark(TaskHDLCHandle),
                 (uint32_t) uxTaskGetStackHighWaterMark(NULL));
        // 待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
