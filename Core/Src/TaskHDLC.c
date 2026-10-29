//
// Created by kikuchi on 2026/10/21.
//
#include <string.h>
#include <stdio.h>
#include "stm32l4xx_hal.h"
#include "cmsis_os.h"
#include "task.h"
#include "queue.h"
#include "main.h"
#include "cmx869b.h"
#include "TaskHDLC.h"
// ********************************************************
// タスク
// ********************************************************
// タスク起床用に自分のハンドルを持ってくる
QueueHandle_t hTxQ, hRxQ;
extern osThreadId_t TaskHDLCHandle;
static void HDLC_service();

// ISRから読んでもらう
void Wakeup_TaskHDLC_FromISR(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(TaskHDLCHandle, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// タスクのエントリポイント。weakをここで上書き
void StartTaskHDLC(void *argument) {
    //キューの作成
    hTxQ = xQueueCreate(10, sizeof(uint8_t));
    hRxQ = xQueueCreate(10, sizeof(uint8_t));
    //CMX869Bの初期化
    cmx869b_init();

    for (;;) {
        // 周期割込待ち
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);;
        // 計測開始
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        //割込処理の実体
        HDLC_service();
        // 計測終了
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}
// QAMステータスの取得(TaskCMX869Bが定期的に読んだ値のコピーを返す。SPIアクセスはしない)
// 1文字送信(送信バッファに詰めて送信開始)
// ステータスを読んで送受信する。割込線が戻るまで繰り返す
static void HDLC_service() {
    StatusRegType st = {0};
    static uint8_t rxBuffer[Q_DATA_SIZE];
    static uint8_t txBuffer[Q_DATA_SIZE];
    static int rxPtr = 0;
    static int txPtr = Q_DATA_SIZE;
    //ステータス確認
    st = cbus_read_status();
    // ------------------------
    // TXレジスタがReadyならデバイスに書く
    // ------------------------
    if (st.Bits.TxDataReady == 1) {
        //txPtrがxQTxの端まで言っていたらキューの有無をしらべて、txPtr=0にする
        if (txPtr == Q_DATA_SIZE) {
            if (xQueueReceive(hTxQ, txBuffer, 0) == pdPASS) {
                txPtr = 0;
            }
        }
        //端までいっていなければ送信
        if (txPtr < Q_DATA_SIZE) {
            cbus_write_data(txBuffer[txPtr++]);
        }
    }
    // ------------------------
    // RXレジスタがReadyなら読む
    // ------------------------
    if (st.Bits.RxDataReady == 1) {
        uint8_t rx_data = cbus_read_data();
        //ポインタが端までいっていなければバッファに書く
        if (rxPtr < 80) {
            rxBuffer[rxPtr++] = rx_data;
        }
        //パケットが終了ならキューに書き込んでrxPtr=0に戻す
        //このときフレームが壊れていればcontrolに書く
        if (st.Bits.FCS == 1) {
            xQueueSend(hRxQ, rxBuffer, 0);
            rxPtr = 0;
        }
    }
}
