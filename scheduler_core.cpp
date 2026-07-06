#include "scheduler_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Функция: create_scheduler
 * Назначение: Создает новый планировщик и инициализирует его
 * Параметры:
 *   processes - массив процессов для планирования
 *   num_processes - количество процессов
 *   time_quantum - квант времени для Round Robin
 * Возвращает: Указатель на созданный планировщик или NULL
 */
Scheduler* create_scheduler(Process** processes, int num_processes, int time_quantum) {
    /* Выделяем память под структуру планировщика */
    Scheduler* s = (Scheduler*)malloc(sizeof(Scheduler));
    if (!s) {
        fprintf(stderr, "Ошибка: не удалось создать планировщик\n");
        return NULL;
    }

    /* Инициализация полей планировщика */
    s->num_processes = num_processes;
    s->time_quantum = time_quantum;
    s->current_time = 0;           /* Начальное время */
    s->running_process = NULL;     /* Нет выполняемого процесса */
    s->num_completed = 0;          /* Нет завершенных процессов */
    s->history_size = 0;
    s->history_capacity = 1000;    /* Начальная вместимость истории */

    /* Выделяем память для копий процессов */
    s->processes = (Process**)malloc(num_processes * sizeof(Process*));
    if (!s->processes) {
        free(s);
        fprintf(stderr, "Ошибка: не удалось выделить память для процессов\n");
        return NULL;
    }

    /* Копируем каждый процесс в планировщик */
    for (int i = 0; i < num_processes; i++) {
        s->processes[i] = (Process*)malloc(sizeof(Process));
        if (!s->processes[i]) {
            /* В случае ошибки освобождаем уже созданные копии */
            for (int j = 0; j < i; j++) free(s->processes[j]);
            free(s->processes);
            free(s);
            return NULL;
        }
        copy_process(s->processes[i], processes[i]);
    }

    /* Сортируем процессы по времени прибытия (для удобства) */
    for (int i = 0; i < num_processes - 1; i++) {
        for (int j = i + 1; j < num_processes; j++) {
            if (s->processes[i]->arrival_time > s->processes[j]->arrival_time) {
                Process* temp = s->processes[i];
                s->processes[i] = s->processes[j];
                s->processes[j] = temp;
            }
        }
    }

    /* Выделяем память для завершенных процессов */
    s->completed = (Process**)malloc(num_processes * sizeof(Process*));
    if (!s->completed) {
        for (int i = 0; i < num_processes; i++) free(s->processes[i]);
        free(s->processes);
        free(s);
        return NULL;
    }

    /* Выделяем память для истории */
    s->history = (HistoryEntry*)malloc(s->history_capacity * sizeof(HistoryEntry));
    if (!s->history) {
        for (int i = 0; i < num_processes; i++) free(s->processes[i]);
        free(s->processes);
        free(s->completed);
        free(s);
        return NULL;
    }

    return s;
}

/*
 * Функция: add_history
 * Назначение: Добавляет запись в историю планирования
 * Параметры:
 *   s - указатель на планировщик
 *   time - текущее время
 *   pid - идентификатор процесса (-1 для простоя)
 *   status - статус ("running" или "idle")
 * Автоматически увеличивает размер истории при необходимости
 */
void add_history(Scheduler* s, int time, int pid, const char* status) {
    if (!s) return;

    /* Если история заполнена, увеличиваем ее размер в 2 раза */
    if (s->history_size >= s->history_capacity) {
        s->history_capacity *= 2;
        s->history = (HistoryEntry*)realloc(s->history, s->history_capacity * sizeof(HistoryEntry));
        if (!s->history) {
            fprintf(stderr, "Ошибка: не удалось увеличить историю\n");
            return;
        }
    }

    /* Заполняем новую запись */
    s->history[s->history_size].time = time;
    s->history[s->history_size].pid = pid;
    strncpy(s->history[s->history_size].status, status, sizeof(s->history[0].status) - 1);
    s->history[s->history_size].status[sizeof(s->history[0].status) - 1] = '\0';
    s->history_size++;
}

/*
 * Функция: free_scheduler
 * Назначение: Освобождает всю память, занятую планировщиком
 * Параметры:
 *   s - указатель на планировщик
 */
void free_scheduler(Scheduler* s) {
    if (!s) return;

    /* Освобождаем все процессы в планировщике */
    for (int i = 0; i < s->num_processes; i++) free(s->processes[i]);
    free(s->processes);

    /* Освобождаем завершенные процессы */
    for (int i = 0; i < s->num_completed; i++) free(s->completed[i]);
    free(s->completed);

    /* Освобождаем историю */
    free(s->history);

    /* Освобождаем сам планировщик */
    free(s);
}

/*
 * Функция: get_process_by_pid
 * Назначение: Находит процесс по его идентификатору
 * Параметры:
 *   s - указатель на планировщик
 *   pid - идентификатор процесса
 * Возвращает: Указатель на процесс или NULL, если процесс не найден
 */
Process* get_process_by_pid(Scheduler* s, int pid) {
    if (!s) return NULL;

    /* Поиск процесса в массиве */
    for (int i = 0; i < s->num_processes; i++) {
        if (s->processes[i]->pid == pid) return s->processes[i];
    }
    return NULL;
}