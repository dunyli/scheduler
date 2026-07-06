#ifndef ROUND_ROBIN_H
#define ROUND_ROBIN_H

#include "scheduler_core.h"

/*
 * Алгоритм Round Robin (круговая очередь)
 * - Вытесняющий
 * - Каждому процессу выделяется квант времени
 * - После истечения кванта процесс переходит в конец очереди
 * - Применение: интерактивные системы, системы разделения времени
 */
Process** round_robin_scheduler(Scheduler* s);

#endif