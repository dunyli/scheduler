#ifndef PROCESS_H
#define PROCESS_H

#include <stdbool.h>

/*
 * Структура процесса
 * Содержит всю информацию для планирования
 */
typedef struct Process {
    int pid;                    /* Уникальный идентификатор */
    char name[20];              /* Имя процесса */
    int arrival_time;           /* Время появления */
    int burst_time;             /* Полное время выполнения */
    int priority;               /* Приоритет (0 - наивысший) */
    int remaining_time;         /* Оставшееся время */
    int start_time;             /* Время начала */
    int completion_time;        /* Время завершения */
    int waiting_time;           /* Время ожидания */
    int turnaround_time;        /* Время оборота */
    char status[20];            /* Статус: готов/выполняется/завершен */
    struct Process* next;       /* Следующий в очереди */
} Process;

/* Создает новый процесс */
Process* create_process(int pid, const char* name, int arrival, int burst, int priority);

/* Копирует процесс */
void copy_process(Process* dest, const Process* src);

/* Освобождает память процесса */
void free_process(Process* p);

/* Выводит информацию о процессе */
void print_process(const Process* p);

/* Сравнивает два процесса по времени прибытия */
bool compare_arrival_time(const Process* a, const Process* b);

/* Генерирует тестовые процессы */
Process** generate_test_processes(int* count);

/* Генерирует случайные процессы */
Process** generate_random_processes(int* count);

/* Освобождает массив процессов */
void free_processes(Process** processes, int count);

#endif