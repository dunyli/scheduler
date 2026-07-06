#ifndef SCHEDULER_CORE_H
#define SCHEDULER_CORE_H

#include "process.h"

/* Запись в истории планирования */
typedef struct HistoryEntry {
    int time;               /* Время события */
    int pid;                /* PID процесса (-1 для простоя) */
    char status[10];        /* Статус: "running" или "idle" */
} HistoryEntry;

/* Основная структура планировщика */
typedef struct Scheduler {
    Process** processes;        /* Массив процессов */
    int num_processes;          /* Количество процессов */
    int time_quantum;           /* Квант времени для RR */
    int current_time;           /* Текущее время */
    Process* running_process;   /* Выполняемый процесс */
    Process** completed;        /* Завершенные процессы */
    int num_completed;          /* Количество завершенных */
    HistoryEntry* history;      /* История для визуализации */
    int history_size;           /* Размер истории */
    int history_capacity;       /* Вместимость истории */
} Scheduler;

/* Создает планировщик */
Scheduler* create_scheduler(Process** processes, int num_processes, int time_quantum);

/* Добавляет запись в историю */
void add_history(Scheduler* s, int time, int pid, const char* status);

/* Освобождает память планировщика */
void free_scheduler(Scheduler* s);

/* Находит процесс по PID */
Process* get_process_by_pid(Scheduler* s, int pid);

#endif