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

void StartTaskHK(void *argument) {
    // 基準時刻の初期化と現在のティック数を取得
    const TickType_t xFrequency = pdMS_TO_TICKS(1000*2); // 1000msをTick数に変換
    TickType_t xLastWakeTime = xTaskGetTickCount();

    //uint8_t f_count = 0;
    //int f_error = 0;

    for (;;) {
        /*
        // 処理開始
        // 回線状態の表示(受信バイト数とQAMステータス)
        uint8_t mode, snr, msg;
        printf("%04d FE:%03d SNR:%d MODE:%02X MSG:%02X ", f_count, f_error, snr, mode, msg);

        //送信実行
        osDelay(200);

        // 受信

        // 1秒待機
        */
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        //f_count++;
    }
}
