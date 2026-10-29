//
// Created by kikuchi on 2026/10/29.
//

#ifndef DRILL_L432_CMX869B_H
#define DRILL_L432_CMX869B_H
// ***********************************************************
// 外部変数定義。番号は適宜変更のこと
// ***********************************************************
extern UART_HandleTypeDef huart2;
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
#define huart huart2
#define hspi hspi1
#define htim htim2
// ***********************************************************
//ステータス・ジレスタビット定義構造体定義
// ***********************************************************
// General Reset
#define General_Reset_ADDR 0x01
// GRE
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
// Status
#define StatusReg_ADDR 0xE6
typedef union {
    uint16_t Word;
    struct {
        uint16_t FSKDemodulatorOutput: 1; // b0
        uint16_t Char2Mode: 1; // b1
        uint16_t Char2EvenHasParity: 1; // b2
        uint16_t FCS: 1; // b3
        uint16_t FrameError: 1; // b4
        uint16_t RxDataOverflow: 1; // b5　 MASK b00
        uint16_t RxDataReady: 1; // b6 　MASK b00
        uint16_t b7_Unused: 1; // b7
        uint16_t b8_RightJustify: 1; // b8
        uint16_t b9_FrameSyncDetect: 1; // b9
        uint16_t EnergyDetect: 1; // b10
        uint16_t TxDataUnderflow: 1; // b11　MASK b03
        uint16_t TxDataReady: 1; // b12　MASK b03
        uint16_t ProgrammingFlag: 1; // b13
        uint16_t RingDetect: 1; // b14
        uint16_t IRQ: 1; // b15
    } __attribute__((packed)) Bits;
} StatusRegType;
// Tx Register
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
// --------------
// Rx Register
// --------------
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

// TxRx Data Register
#define TxData_ADDR 0xE4   //8bit
#define RxData_ADDR 0xE5   //8bit

// QAM Register
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

// --------------
// QAM Status Register
// --------------
#define QamStatusReg_ADDR 0xEB
typedef union {
    uint16_t Word;
    struct {
        uint16_t Mode: 4; //b0
        uint16_t SNR: 3;
        uint16_t Zeros: 3;
        uint16_t Messages: 6;
    } __attribute__((packed)) Bits;
} QamStatusRegType;

// ***********************************************************
// ステータス・ジレスタビット定義構造体定義
// ***********************************************************
void cbus_read_write_raw(const uint8_t tx_data[], uint8_t rx_data[], int is_isr);
//
void cbus_write_reg(uint8_t adr, uint16_t data);
void cbus_write_data(uint8_t data);
uint16_t cbus_read_reg(uint8_t adr);
uint8_t cbus_read_data(void);
//
void qam_monitor(int is_gse, int tic);
void cmx_init(int is_gse_mode);
void cmx_service(void);
void cbus_read_write(const uint8_t tx_data[], uint8_t rx_data[], int len);
void set_bell(void);
void set_qam_call(void);
void set_qam_answer(void);
void set_bps(uint8_t mode);
//
void int_log(int count);

#endif //DRILL_L432_CMX869B_H
