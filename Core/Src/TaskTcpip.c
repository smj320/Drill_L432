//
// Created by kikuchi on 2026/10/21.
//
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "cmsis_os.h"
#include "task.h"
#include "stm32l4xx_hal.h"
#include "TaskTcpip.h"
#include "TaskCMX869B_priv.h"

GREType GRE = {0};
StatusRegType StatusReg = {0};
TxRegType TxReg = {0};
RxRegType RxReg = {0};
QamRegType QamReg = {0};
QamStatusRegType QamStatusReg = {0};

QueueHandle_t xQueueTx = NULL;
QueueHandle_t xQueueRxHK = NULL;
QueueHandle_t xQueueRxIMG = NULL;
QueueHandle_t xQueueRxCMD = NULL;

int Is_gse_mode = 0;
uint32_t N_poll_1sec = 0;

// ********************************************************
// 公開関数
// ********************************************************
// QAMステータスの取得(TaskCMX869Bが定期的に読んだ値のコピーを返す。SPIアクセスはしない)
void CMX869B_get_qam_status(uint8_t *mode, uint8_t *snr, uint8_t *msg) {
    //QAMのステータを取る
    *mode = QamStatusReg.Bits.Mode;
    *snr = QamStatusReg.Bits.SNR;
    *msg = QamStatusReg.Bits.Messages;
}

// 1文字送信(送信バッファに詰めて送信開始)
void CMX869B_putc(uint8_t tx_data) {
    cbus_write_data(tx_data);
}
// ステータスを読んで送受信する。割込線が戻るまで繰り返す
void cmx_service() {
    static int count = 0;
    static xQueuePacketType txQ;
    // 受信は関数呼び出しをまたいで1フレーム分ためるのでstaticにする
    static uint8_t rxBuffer[HDLC_HEADER_SIZE + Q_DATA_SIZE + FSC_SIZE];
    static int rxPtr = 0;
    static int txPtr = HDLC_HEADER_SIZE + Q_DATA_SIZE;

    //計測開始
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
    //
    //ステータス確認
    StatusReg.Word = cbus_read_reg(StatusReg_ADDR);
    //
    // ------------------------
    // TXレジスタがReadyならデバイスに書く
    // ------------------------
    if (StatusReg.Bits.TxDataReady == 1) {
        //txPtrがxQTxの端まで言っていたらキューの有無をしらべて、txPtr=0にする
        if (txPtr == HDLC_HEADER_SIZE + Q_DATA_SIZE) {
            if (xQueueReceive(xQueueTx, &txQ, 0) == pdPASS) {
                txPtr = 0;
            }
        }
        //端までいっていなければ送信
        if (txPtr < HDLC_HEADER_SIZE + Q_DATA_SIZE) {
            cbus_write_data(((uint8_t *) &txQ)[txPtr++]);
        }
    }
    // ------------------------
    // RXレジスタがReadyなら読む
    // ------------------------
    if (StatusReg.Bits.RxDataReady == 1) {
        uint8_t rx_data = cbus_read_data();
        //ポインタが端までいっていなければバッファに書く
        if (rxPtr < HDLC_HEADER_SIZE + Q_DATA_SIZE + FSC_SIZE) {
            rxBuffer[rxPtr++] = rx_data;
        }
        //パケットが終了ならキューに書き込んでrxPtr=0に戻す
        //このときフレームが壊れていればcontrolに書く
        if (StatusReg.Bits.FCS == 1) {
            xQueuePacketType rxQ;
            memcpy(&rxQ, rxBuffer, HDLC_HEADER_SIZE + Q_DATA_SIZE);
            rxQ.control.isAbort = StatusReg.Bits.FrameError;
            xQueueSend(xQueueRxHK, &rxQ, 0);
            rxPtr = 0;
        }
    }
    // 計測終了
    HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    // 一秒に一回更新
    if (count++ == N_poll_1sec) {
        QamStatusReg.Word = cbus_read_reg(QamStatusReg_ADDR);
        count = 0;
    }
}

// ****************************************************
// Externで外部に出す関数
// ****************************************************
// タスクの実体
#define PKT_QUEUE_DEPTH 4
#define CMX_POLL_MS 5
#define CMX_QAM_STATUS_MS 200
// 他タスク参照用のQAMステータスのコピー(16bitなので読み書きはアトミック)
static volatile uint16_t qam_status_word = 0;

void StartTaskCMX869B(void *argument) {
    // パケット単位のキューの作成(1要素 = 1パケット)
    xQueueTx = xQueueCreate(PKT_QUEUE_DEPTH, sizeof(xQueuePacketType));
    xQueueRxHK = xQueueCreate(PKT_QUEUE_DEPTH, sizeof(xQueuePacketType));

    //ジャンパークローズ(0)の場合はGSE
    Is_gse_mode = HAL_GPIO_ReadPin(MODEM_MODE_GPIO_Port, MODEM_MODE_Pin) == 0 ? 1 : 0;

    //CMX869Bの初期化
    cmx_init(Is_gse_mode);

    // ********************************************************
    // 待機処理
    // ********************************************************
    // 割込(ISRからの通知)またはタイムアウトで起きてステータスを処理する
    // タイムアウトは割込の取りこぼし対策のポーリングを兼ねる
    // CMX869BへのSPIアクセスはこのタスクだけが行う
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);;
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_SET);
        //割込処理の実体
        cmx_service();
        HAL_GPIO_WritePin(CPU_MON_GPIO_Port, CPU_MON_Pin, GPIO_PIN_RESET);
    }
}


// printf用
void uart_putc(unsigned char c) {
    while (!(huart.Instance->ISR & USART_ISR_TXE)) {
    }
    huart.Instance->TDR = c;
}
