//
// Created by kikuchi on 2026/10/21.
//

#include <sys/types.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "TaskHK.h"
#include "TaskHDLC.h"
//
extern  QueueHandle_t hTxQ, hRxQ;

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
            printf("GSE %d : \r\n", Is_gse);
            for (int i = 0; i < Q_DATA_SIZE; i++) {
                printf("%04d ",rx_data[i]);
            }
            printf("\r\n");
        }else {
            printf("Receive timeout\r\n");
        }
        // 待機
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        f_count++;
    }
}
