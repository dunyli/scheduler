#include "fifo.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Структура очереди для FIFO */
typedef struct QueueNode {
    Process* process;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;  /* Начало очереди */
    QueueNode* rear;   /* Конец очереди */
    int size;          /* Размер очереди */
} Queue;

/*
 * Функция: create_queue
 * Назначение: Создает пустую очередь
 * Возвращает: Указатель на созданную очередь
 */
static Queue* create_queue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

/*
 * Функция: enqueue
 * Назначение: Добавляет процесс в конец очереди
 * Параметры:
 *   q - указатель на очередь
 *   p - указатель на процесс
 */
static void enqueue(Queue* q, Process* p) {
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    if (!node) return;
    node->process = p;
    node->next = NULL;

    /* Если очередь пуста, новый элемент становится и началом, и концом */
    if (q->rear == NULL) {
        q->front = q->rear = node;
    }
    else {
        /* Иначе добавляем в конец */
        q->rear->next = node;
        q->rear = node;
    }
    q->size++;
}

/*
 * Функция: dequeue
 * Назначение: Извлекает процесс из начала очереди
 * Параметры:
 *   q - указатель на очередь
 * Возвращает: Указатель на извлеченный процесс
 */
static Process* dequeue(Queue* q) {
    if (q->front == NULL) return NULL;

    QueueNode* node = q->front;
    Process* p = node->process;
    q->front = q->front->next;

    /* Если очередь стала пустой, обнуляем указатель на конец */
    if (q->front == NULL) q->rear = NULL;

    free(node);
    q->size--;
    return p;
}

/*
 * Функция: is_queue_empty
 * Назначение: Проверяет, пуста ли очередь
 * Параметры:
 *   q - указатель на очередь
 * Возвращает: true если пуста, иначе false
 */
static bool is_queue_empty(Queue* q) {
    return q->front == NULL;
}

/*
 * Функция: free_queue
 * Назначение: Освобождает память, занятую очередью
 * Параметры:
 *   q - указатель на очередь
 */
static void free_queue(Queue* q) {
    while (!is_queue_empty(q)) dequeue(q);
    free(q);
}

/*
 * Функция: fifo_scheduler
 * Назначение: Запускает алгоритм планирования FIFO
 * Параметры:
 *   s - указатель на планировщик
 * Возвращает: Массив завершенных процессов
 *
 * Алгоритм работы:
 * 1. Процессы добавляются в очередь по времени прибытия
 * 2. Выполняется первый процесс из очереди
 * 3. Процесс выполняется до завершения (невытесняющий)
 * 4. После завершения берется следующий процесс
 */
Process** fifo_scheduler(Scheduler* s) {
    if (!s) return NULL;

    Queue* ready_queue = create_queue();  /* Очередь готовых процессов */
    if (!ready_queue) return NULL;

    int remaining_count = s->num_processes;  /* Сколько процессов осталось выполнить */
    int process_index = 0;  /* Индекс следующего добавляемого процесса */

    /* Основной цикл планирования */
    while (remaining_count > 0 || !is_queue_empty(ready_queue) || s->running_process != NULL) {
        /* Добавляем новые процессы в очередь по времени прибытия */
        while (process_index < s->num_processes &&
            s->processes[process_index]->arrival_time <= s->current_time) {
            enqueue(ready_queue, s->processes[process_index]);
            process_index++;
        }

        /* Если нет выполняющегося процесса, берем первый из очереди */
        if (s->running_process == NULL && !is_queue_empty(ready_queue)) {
            s->running_process = dequeue(ready_queue);

            /* Запоминаем время начала выполнения */
            if (s->running_process->start_time == -1) {
                s->running_process->start_time = s->current_time;
            }
            strcpy(s->running_process->status, "выполняется");
        }

        /* Выполняем процесс (один такт) */
        if (s->running_process != NULL) {
            s->running_process->remaining_time--;
            strcpy(s->running_process->status, "выполняется");
            add_history(s, s->current_time, s->running_process->pid, "running");

            /* Проверяем, завершился ли процесс */
            if (s->running_process->remaining_time == 0) {
                /* Расчет времени завершения и статистики */
                s->running_process->completion_time = s->current_time + 1;
                s->running_process->turnaround_time =
                    s->running_process->completion_time - s->running_process->arrival_time;
                s->running_process->waiting_time =
                    s->running_process->turnaround_time - s->running_process->burst_time;
                strcpy(s->running_process->status, "завершен");

                /* Сохраняем завершенный процесс */
                Process* completed = (Process*)malloc(sizeof(Process));
                if (completed) {
                    copy_process(completed, s->running_process);
                    s->completed[s->num_completed++] = completed;
                }

                s->running_process = NULL;
                remaining_count--;
            }
        }
        else {
            /* Если процесс не выполняется - простой процессора */
            add_history(s, s->current_time, -1, "idle");
        }

        s->current_time++;  /* Увеличиваем время */
    }

    free_queue(ready_queue);
    return s->completed;
}