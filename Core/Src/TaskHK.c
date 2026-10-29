//
// Created by kikuchi on 2026/10/21.
//
#include "main.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "task.h"
#include "TaskHK.h"

#include <sys/types.h>

#include "TaskHK_priv.h"
#include "TaskCMX869B.h"
#include "TaskCMX869B_priv.h"

void TaskHKEntry(void *argument) {
    xQueuePacketType xQTx, xQRx;

    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    uint8_t f_count = 0;
    int f_error = 0;

    xQTx.control.addr = HDLC_ADDR_HK;
    xQTx.control.pos = HDLC_POS_SINGLE;
    xQTx.control.isAbort = 0;
    for (;;) {
        // 処理開始
        // 回線状態の表示(受信バイト数とQAMステータス)
        uint8_t mode, snr, msg;
        CMX869B_get_qam_status(&mode, &snr, &msg);
        printf("%04d FE:%03d SNR:%d MODE:%02X MSG:%02X ", f_count, f_error, snr, mode, msg);

        //データの準備
        xQTx.count = f_count;
        xQTx.len  = Q_DATA_SIZE;
        for (uint8_t i = 0; i < Q_DATA_SIZE; i++) {
            xQTx.data[i] = i;
        }

        //送信実行
        xQueueSend(xQueueTx, &xQTx, 0);
        osDelay(200);

        // 受信
        if (xQueueReceive(xQueueRxHK, &xQRx, pdMS_TO_TICKS(1500)) == pdPASS) {
            if (xQRx.control.isAbort == 1) {
                f_error++;
                printf("__ ");
            }
            printf("%02X %02X ", xQRx.len, xQRx.count);
            for (int i = 0; i < xQRx.len; i++) {
                printf("%04d ", xQRx.data[i]);
            }
            printf("\r\n");
        } else {
            printf("TimeOut\r\n");
        }

        // 1秒待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
