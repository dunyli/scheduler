#ifndef PRIORITY_H
#define PRIORITY_H

#include "scheduler_core.h"

/*
 * Алгоритм приоритетного планирования с несколькими очередями
 * - Вытесняющий
 * - Поддержка 4 уровней приоритета (0-3)
 * - Приоритетное ускорение (boost) каждые 20 тактов
 * - Применение: системы реального времени, мультимедийные системы
 */
Process** priority_scheduler(Scheduler* s);

#endif