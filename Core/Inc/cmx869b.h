//
// Created by kikuchi on 2026/10/29.
//

#ifndef DRILL_L432_CMX869B_H
#define DRILL_L432_CMX869B_H
// ----------------------------------------------------------------------------
// 外部変数定義。番号は適宜変更のこと
// ----------------------------------------------------------------------------
extern SPI_HandleTypeDef hspi1;
extern TIM_HandleTypeDef htim2;
#define hspi hspi1
#define htim htim2
// Status Register
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
// QAM Status Register
#define StatusRegQam_ADDR 0xEB
typedef union {
    uint16_t Word;
    struct {
        uint16_t Mode: 4; //b0
        uint16_t SNR: 3;
        uint16_t Zeros: 3;
        uint16_t Messages: 6;
    } __attribute__((packed)) Bits;
} StatusQamRegType;

// ----------------------------------------------------------------------------
// ステータス・ジレスタビット定義構造体定義
// ----------------------------------------------------------------------------
void cbus_write_data(uint8_t data);
uint8_t cbus_read_data(void);
StatusRegType cbus_read_status(void);
StatusQamRegType cbus_read_status_qam(void);
void cmx869b_init();
//
#endif //DRILL_L432_CMX869B_H
