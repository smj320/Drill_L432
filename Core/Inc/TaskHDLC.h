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
    int lastLen;
} HdlcCntType;
extern HdlcCntType HdlcCnt;

#endif //CMX869B_TASK_HDLC_H
