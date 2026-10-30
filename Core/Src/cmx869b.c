//
// Created by kikuchi on 2026/10/29.
//
#include <stdint.h>
#include <stdio.h>
#include "cmsis_os.h"
#include "task.h"
#include "main.h"
#include "cmx869b.h"
// ----------------------------------------------------------------------------
//ステータス・ジレスタビット定義構造体定義
// ----------------------------------------------------------------------------
// General Reset
#define General_Reset_ADDR 0x01

// Global Mode Register
#define GRE_ADDR 0xE0
typedef union {
    uint16_t Word;
    struct {
        uint16_t IrqMask: 6; //b0
        uint16_t IrqEna: 1;
        uint16_t Rst: 1;
        uint16_t Pwr: 1;
        uint16_t RlyDrv: 1;
        uint16_t _2C: 1;
        uint16_t LB: 1;
        uint16_t Zero: 1;
        uint16_t PatDet: 1;
        uint16_t HighGain: 1;
        uint16_t Equ: 1;
    } __attribute__((packed)) Bits;
} GREType;

// Tx Mode Register
#define TxReg_ADDR 0xE1
#define TxReg_Mode_QAM_AUTO 0b1111
#define TxReg_Mode_V22_ANS 0b1011
#define TxReg_Mode_V22_CALL 0b1010
#define TxReg_Mode_BELL 0b0011
typedef union {
    uint16_t Word;
    struct {
        uint16_t DataBits: 3; //b0
        uint16_t StartStop: 2;
        uint16_t Scramble: 2;
        uint16_t GardTone: 2;
        uint16_t TxLevel: 3;
        uint16_t TxMode: 4;
    } __attribute__((packed)) Bits;
} TxRegType;

// Rx Mode Register
#define RxReg_ADDR 0xE2
#define RxReg_Mode_QAM_AUTO 0b1111
#define RxReg_Mode_V22_CALL 0b1011
#define RxReg_Mode_V22_ANS 0b1010
#define RxReg_Mode_BELL 0b0011
typedef union {
    uint16_t Word;
    struct {
        uint16_t BitsParity: 3; //b0
        uint16_t StartStop_Synch: 3;
        uint16_t Equaliser: 3;
        uint16_t RxLevel: 3;
        uint16_t RxMode: 4;
    } __attribute__((packed)) Bits;
} RxRegType;

// QAM Mode Register
#define QamCmdReg_ADDR 0xEA
#define QamMaxBR_14400 0b111
#define QamMaxBR_12000 0b110
#define QamMaxBR_9600 0b101
#define QamMaxBR_2400 0b001
#define QamCall 0b010
#define QamAnswer 0b011
#define QamZeros 0b0000000000
typedef union {
    uint16_t Word;
    struct {
        uint16_t br: 3;
        uint16_t command: 3; //Calling/Answer
        uint16_t zeros: 10;
    } __attribute__((packed)) Bits;
} QamRegType;

// TxRx Data Register
#define TxData_ADDR 0xE4   //8bit
#define RxData_ADDR 0xE5   //8bit

// ----------------------------------------------------------------------------
// C Bus 操作関数
// ----------------------------------------------------------------------------
// TaskCMX869Bからのみ呼ぶこと(ISR・他タスクからは呼ばない)
static void cbus_read_write(const uint8_t tx_data[], uint8_t rx_data[], int len) {
    //CS Low
    taskENTER_CRITICAL();
    C_CS_GPIO_Port->BSRR = C_CS_Pin << 16;
    // 2. 3バイト分をダイレクトに送受信する（例としてSPI1を使用）
    for (int i = 0; i < len; i++) {
        // 送信バッファが空になるのを待つ
        while (!(SPI1->SR & SPI_FLAG_TXE));
        // データを書き込む（書き込んだ瞬間にクロックが出始める）
        *(volatile uint8_t *) (&SPI1->DR) = tx_data[i];

        // 受信データが完了するのを待つ
        while (!(SPI1->SR & SPI_FLAG_RXNE));
        // 受信データを読み出す
        rx_data[i] = *(volatile uint8_t *) (&SPI1->DR);
    }
    //CS High
    C_CS_GPIO_Port->BSRR = C_CS_Pin;
    taskEXIT_CRITICAL();
}

static void cbus_write_reg(uint8_t adr, uint16_t data) {
    uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = (data >> 8) & 0xFF;
    tx_data[2] = (data >> 0) & 0xFF;
    cbus_read_write(tx_data, rx_data, 3);
}

static uint16_t cbus_read_reg(uint8_t adr) {
    uint8_t tx_data[3], rx_data[3];
    tx_data[0] = adr;
    tx_data[1] = tx_data[2] = 0x0;
    cbus_read_write(tx_data, rx_data, 3);
    uint16_t result = (rx_data[1] << 8) + rx_data[2];
    return result;
}

// ----------------------------------------------------------------------------
// C Bus 公開 操作関数
// ----------------------------------------------------------------------------
void cbus_write_data(uint8_t data) {
    uint8_t tx_data[2], rx_data[2];
    tx_data[0] = TxData_ADDR;
    tx_data[1] = data;
    cbus_read_write(tx_data, rx_data, 2);
}

uint8_t cbus_read_data(void) {
    uint8_t tx_data[2], rx_data[2];
    tx_data[0] = RxData_ADDR;
    tx_data[1] = 0x0;
    cbus_read_write(tx_data, rx_data, 2);
    return rx_data[1];
}

StatusRegType cbus_read_status(void) {
    StatusRegType st;
    st.Word = cbus_read_reg(StatusReg_ADDR);
    return st;
}

StatusQamRegType cbus_read_status_qam(void) {
    StatusQamRegType qamSt;
    qamSt.Word = cbus_read_reg(StatusRegQam_ADDR);
    return qamSt;
}

static StatusQamRegType cbus_monitor_status_qam() {
    static int tic = 0;
    StatusQamRegType qamSt= {0};
    qamSt = cbus_read_status_qam();
    printf("GSE:%d TI:%03d MSG:%02X SNR:%02X MODE:%02X\r\n",
            Is_gse,
            tic, qamSt.Bits.Messages,
            qamSt.Bits.SNR,
            qamSt.Bits.Mode);
    tic++;
    HAL_Delay(500);
    return qamSt;
}
// ----------------------------------------------------------------------------
// モデムモード設定関数
// ----------------------------------------------------------------------------
#define TX_LEVEL 0b101;
#define RX_LEVEL 0b111;
// ------------------------------------
// 調整用
// ------------------------------------
static void set_bell() {
    TxRegType txReg= {0};
    RxRegType rxReg = {0};
    // TX
    txReg.Bits.TxMode = TxReg_Mode_BELL;
    txReg.Bits.TxLevel = TX_LEVEL;
    //txReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    //txReg.Bits.DataBits = 0b111; //8bit Stop2
    txReg.Bits.StartStop = 0b11; //Special Mode
    txReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, txReg.Word);
    // RX
    rxReg.Bits.RxMode = RxReg_Mode_BELL;
    rxReg.Bits.RxLevel = RX_LEVEL;
    //rxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    //rxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    rxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    rxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, rxReg.Word);
}
// ------------------------------------
// AutoModem ネゴシエーション必須
// ------------------------------------
uint8_t MaxSpeed = QamMaxBR_14400;
//uint8_t MaxSpeed = QamMaxBR_12000;
//uint8_t MaxSpeed = QamMaxBR_9600;
//uint8_t MaxSpeed = QamMaxBR_2400;
//
static void set_qam_answer() {
    TxRegType txReg= {0};
    RxRegType rxReg = {0};
    QamRegType qamReg = {0};
    // TX
    txReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    txReg.Bits.TxLevel = TX_LEVEL;
    //txReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    //txReg.Bits.DataBits = 0b111; //8bit Stop2
    txReg.Bits.StartStop = 0b11; //Special Mode
    txReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, txReg.Word);
    HAL_Delay(10);
    // RX
    rxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    rxReg.Bits.RxLevel = RX_LEVEL;
    //rxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    //rxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    rxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    rxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, rxReg.Word);
    HAL_Delay(10);
    //QAM
    qamReg.Bits.br = MaxSpeed;
    qamReg.Bits.command = QamAnswer;
    qamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, qamReg.Word);
    HAL_Delay(10);
}

static void set_qam_call() {
    TxRegType txReg= {0};
    RxRegType rxReg = {0};
    QamRegType qamReg = {0};
    // TX
    txReg.Bits.TxMode = TxReg_Mode_QAM_AUTO;
    txReg.Bits.TxLevel = TX_LEVEL;
    //txReg.Bits.StartStop = 0b10; //Start-stop, NonParity
    //txReg.Bits.DataBits = 0b111; //8bit Stop2
    txReg.Bits.StartStop = 0b11; //Special Mode
    txReg.Bits.DataBits = 0b110; //HDLC Mode
    cbus_write_reg(TxReg_ADDR, txReg.Word);
    HAL_Delay(10);
    // RX
    rxReg.Bits.RxMode = RxReg_Mode_QAM_AUTO;
    rxReg.Bits.RxLevel = RX_LEVEL;
    //rxReg.Bits.StartStop_Synch = 0b110; //Start-stop, NonOverSpeed
    //rxReg.Bits.BitsParity = 0b110; //8bit, NonParity (0b111は8bit+Parity)
    rxReg.Bits.StartStop_Synch = 0b111; //Special Mode
    rxReg.Bits.BitsParity = 0b110; //HDLC Mode
    cbus_write_reg(RxReg_ADDR, rxReg.Word);
    HAL_Delay(10);
    //QAM
    qamReg.Bits.br = MaxSpeed;
    qamReg.Bits.command = QamCall;
    qamReg.Bits.zeros = QamZeros;
    cbus_write_reg(QamCmdReg_ADDR, qamReg.Word);
    HAL_Delay(10);
}
// ****************************************************************************
// 初期化
// ****************************************************************************
// モードから周期割込の頻度を決める
static void set_poll_count(uint8_t mode) {
    // TIM2のクロック。SYSCLKはMSI 4MHz x16 /2 = 32MHz。
    // APB1が1分周なのでTIM2クロック = PCLK1(分周するとx2になるので注意)
    const uint32_t TIM_CLK = HAL_RCC_GetPCLK1Freq();
    uint32_t target_modes[] = {0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08};
    uint32_t target_freqs[] = {14400, 12000, 9600, 9600, 7200, 4800, 2400, 1200};
    uint32_t target_freq = 1200;

    //モード判定
    for (uint8_t i = 0; i < 8; i++) {
        if (target_modes[i] == mode) {
            target_freq = target_freqs[i];
        }
    }

    // この tim2_clock を使って 14400Hz の計算をする
    uint32_t booClk = (TIM_CLK / target_freq);
    // booClkがbpsに相当するので、このルーチンをbClk回抜けると1秒になる
    // 4bitの時間おきにポーリングにいく
    __HAL_TIM_SET_AUTORELOAD(&htim2, booClk*3-1);

    //割込開始
    HAL_TIM_Base_Start_IT(&htim2);
}
// ------------------------------------
// キモの初期化
// ------------------------------------
/*
// ハンドシェーク監視時間(500ms単位)
#define HANDSHAKE_TIC 20
// QAMステータスのModeがこの値以上なら接続済み(set_poll_countのtarget_modesと同じ範囲)
#define QAM_MODE_CONNECTED 0x08
*/
void cmx869b_init() {
    GREType gre = {0};
    StatusRegType st = {0};
    StatusQamRegType stqam = {0};

    static int is_loop_back = 0;
    static int is_Cal = 0;

    // 動作モード選択
    is_loop_back = 0;
    is_Cal = 0;

    // SPIイネーブル
    __HAL_SPI_ENABLE(&hspi);

    // グローバルリセット
    cbus_write_reg(General_Reset_ADDR, 0);

    // ステータス確認
    st = cbus_read_status();
    (void) st; //コンパイル時の不使用警告を抑止

    // Pwr=1,Rst=1で20ms以上保持して内部ロジックを確定させる(データシート GCR b8)
    // ここで水晶が発振する。
    gre.Bits.Pwr = 1;
    gre.Bits.Rst = 1;
    cbus_write_reg(GRE_ADDR, gre.Word);
    st = cbus_read_status();
    HAL_Delay(25);

    //send_cmdで、うまくいくと22pinが発振する
    //gre.Bits.HighGain = 1;
    gre.Bits.HighGain = 0;
    gre.Bits.PatDet = 1;
    gre.Bits.LB = is_loop_back == 1 ? 1 : 0;
    gre.Bits.Rst = 0;
    gre.Bits.IrqEna = 1;
    gre.Bits.IrqMask = 0b000000;
    cbus_write_reg(GRE_ADDR, gre.Word);
    st = cbus_read_status();
    HAL_Delay(1);

    // モデム送受信の設定
    if (is_loop_back == 1) {
        set_bell();
        if (is_Cal == 1) {
            while (1);
        }
    } else {
        // Answer側は接続するまで待ち受け続ける。
        // Call側はHANDSHAKE_TIC(500ms単位)で接続できなければ発呼からやり直す。
        // これで立ち上げ順に依存せずにハンドシェークする
        //int connected = 0;
        if (Is_gse==1) {
            set_qam_call();
        }else {
            set_qam_answer();
        }
        for (uint8_t tic = 0; tic < 60; tic++) {
            cbus_monitor_status_qam();
            HAL_Delay(500);
        }
    }
    // ポーリングビットレート設定
    st = cbus_read_status();
    stqam = cbus_read_status_qam();
    set_poll_count(stqam.Bits.Mode);
    // 合図
    printf("Initialized\r\n");
}