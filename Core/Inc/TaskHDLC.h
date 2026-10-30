//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASK_HDLC_H
#define CMX869B_TASK_HDLC_H

#define Q_DATA_SIZE 80

void HDLC_QueueInit(void);
void StartTaskHDLC(void *argument);
void Wakeup_TaskHDLC_FromISR(void);

extern int FrameError;

// 診断用カウンタ
typedef struct {
    uint32_t rxReady;
    uint32_t rxOverflow;
    uint32_t txUnderflow;
    uint32_t frameError;
    uint32_t fcs;
    uint32_t txFrame;   // キューから取り出した送信フレーム数
    uint32_t service;   // HDLC_serviceの実行回数
    uint32_t fcsWithErr; // 終端時にFrameErrorも立っていた回数
    uint32_t shortFrame; // 長さがQ_DATA_SIZEでなかったフレーム数
    uint32_t rxQFull;    // 受信キュー満杯で捨てたフレーム数
    int lastLen;
} HdlcCntType;
extern HdlcCntType HdlcCnt;

#endif //CMX869B_TASK_HDLC_H
