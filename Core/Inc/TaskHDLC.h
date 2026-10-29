//
// Created by kikuchi on 2026/10/23.
//

#ifndef CMX869B_TASKCMX869B_H
#define CMX869B_TASKCMX869B_H

#define Q_DATA_SIZE 80

void StartTaskHDLC(void *argument);
void Wakeup_TaskHDLC_FromISR(void);

#endif //CMX869B_TASKCMX869B_H
