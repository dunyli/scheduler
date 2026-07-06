#ifndef FIFO_H
#define FIFO_H

#include "scheduler_core.h"

/*
 * Алгоритм FIFO (First In First Out)
 * - Невытесняющий
 * - Процессы выполняются в порядке поступления
 * - Применение: пакетная обработка, очереди печати
 */
Process** fifo_scheduler(Scheduler* s);

#endif