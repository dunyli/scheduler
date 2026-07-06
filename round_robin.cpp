#include "round_robin.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Структура очереди для Round Robin */
typedef struct QueueNode {
    Process* process;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

/*
 * Функции работы с очередью (аналогичны FIFO)
 */
static Queue* create_queue() {
    Queue* q = (Queue*)malloc(sizeof(Queue));
    if (!q) return NULL;
    q->front = q->rear = NULL;
    q->size = 0;
    return q;
}

static void enqueue(Queue* q, Process* p) {
    QueueNode* node = (QueueNode*)malloc(sizeof(QueueNode));
    if (!node) return;
    node->process = p;
    node->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = node;
    }
    else {
        q->rear->next = node;
        q->rear = node;
    }
    q->size++;
}

static Process* dequeue(Queue* q) {
    if (q->front == NULL) return NULL;

    QueueNode* node = q->front;
    Process* p = node->process;
    q->front = q->front->next;

    if (q->front == NULL) q->rear = NULL;

    free(node);
    q->size--;
    return p;
}

static bool is_queue_empty(Queue* q) {
    return q->front == NULL;
}

static void free_queue(Queue* q) {
    while (!is_queue_empty(q)) dequeue(q);
    free(q);
}

/*
 * Функция: round_robin_scheduler
 * Назначение: Запускает алгоритм Round Robin
 * Параметры:
 *   s - указатель на планировщик
 * Возвращает: Массив завершенных процессов
 *
 * Алгоритм работы:
 * 1. Процессы добавляются в очередь по времени прибытия
 * 2. Выполняется процесс из начала очереди
 * 3. После истечения кванта времени процесс вытесняется
 * 4. Вытесненный процесс помещается в конец очереди
 * 5. Выполняется следующий процесс
 */
Process** round_robin_scheduler(Scheduler* s) {
    if (!s) return NULL;

    Queue* ready_queue = create_queue();
    if (!ready_queue) return NULL;

    int remaining_count = s->num_processes;
    int process_index = 0;
    int time_slice = 0;  /* Счетчик использованного кванта времени */

    while (remaining_count > 0 || !is_queue_empty(ready_queue) || s->running_process != NULL) {
        /* Добавляем новые процессы в очередь */
        while (process_index < s->num_processes &&
            s->processes[process_index]->arrival_time <= s->current_time) {
            enqueue(ready_queue, s->processes[process_index]);
            process_index++;
        }

        /* Если нет выполняющегося, берем из очереди */
        if (s->running_process == NULL && !is_queue_empty(ready_queue)) {
            s->running_process = dequeue(ready_queue);
            if (s->running_process->start_time == -1) {
                s->running_process->start_time = s->current_time;
            }
            strcpy(s->running_process->status, "выполняется");
            time_slice = 0;  /* Сброс счетчика для нового процесса */
        }

        /* Выполняем процесс */
        if (s->running_process != NULL) {
            s->running_process->remaining_time--;
            strcpy(s->running_process->status, "выполняется");
            add_history(s, s->current_time, s->running_process->pid, "running");
            time_slice++;

            /* Проверяем завершение */
            if (s->running_process->remaining_time == 0) {
                s->running_process->completion_time = s->current_time + 1;
                s->running_process->turnaround_time =
                    s->running_process->completion_time - s->running_process->arrival_time;
                s->running_process->waiting_time =
                    s->running_process->turnaround_time - s->running_process->burst_time;
                strcpy(s->running_process->status, "завершен");

                Process* completed = (Process*)malloc(sizeof(Process));
                if (completed) {
                    copy_process(completed, s->running_process);
                    s->completed[s->num_completed++] = completed;
                }

                s->running_process = NULL;
                remaining_count--;
                time_slice = 0;
            }
            else if (time_slice >= s->time_quantum) {
                /* Квант времени истек - вытеснение */
                strcpy(s->running_process->status, "готов");
                enqueue(ready_queue, s->running_process);
                s->running_process = NULL;
                time_slice = 0;
            }
        }
        else {
            add_history(s, s->current_time, -1, "idle");
        }

        s->current_time++;
    }

    free_queue(ready_queue);
    return s->completed;
}