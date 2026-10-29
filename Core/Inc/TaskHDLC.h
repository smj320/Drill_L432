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

#endif //CMX869B_TASK_HDLC_H
