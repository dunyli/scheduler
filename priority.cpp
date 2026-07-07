#define _CRT_SECURE_NO_WARNINGS
#include "priority.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Структура очереди */
typedef struct QueueNode {
    Process* process;
    struct QueueNode* next;
} QueueNode;

typedef struct {
    QueueNode* front;
    QueueNode* rear;
    int size;
} Queue;

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
 * Функция: priority_scheduler
 * Назначение: Запускает алгоритм приоритетного планирования
 *
 * ИСПРАВЛЕННАЯ ВЕРСИЯ:
 * - Процессы с более высоким приоритетом (меньший номер) выполняются раньше
 * - При появлении процесса с более высоким приоритетом происходит вытеснение
 */
Process** priority_scheduler(Scheduler* s) {
    if (!s) return NULL;

    /* Создаем 4 очереди для разных уровней приоритета (0 - наивысший) */
    Queue* queues[4];
    for (int i = 0; i < 4; i++) {
        queues[i] = create_queue();
        if (!queues[i]) {
            for (int j = 0; j < i; j++) free_queue(queues[j]);
            return NULL;
        }
    }

    int remaining_count = s->num_processes;
    int process_index = 0;
    int boost_counter = 0;

    while (remaining_count > 0 ||
        !is_queue_empty(queues[0]) || !is_queue_empty(queues[1]) ||
        !is_queue_empty(queues[2]) || !is_queue_empty(queues[3]) ||
        s->running_process != NULL) {

        /* Добавляем новые процессы в очереди в зависимости от приоритета */
        while (process_index < s->num_processes &&
            s->processes[process_index]->arrival_time <= s->current_time) {
            int priority = s->processes[process_index]->priority;
            /* Ограничиваем приоритет диапазоном 0-3 */
            if (priority > 3) priority = 3;
            if (priority < 0) priority = 0;

            /* Добавляем в очередь соответствующего приоритета */
            enqueue(queues[priority], s->processes[process_index]);
            process_index++;
        }

        /*
         * Приоритетное ускорение (Priority Boosting)
         * Каждые 20 тактов все процессы перемещаются в очередь 0
         * Это предотвращает голодание процессов с низким приоритетом
         */
        boost_counter++;
        if (boost_counter >= 20) {
            boost_counter = 0;
            for (int level = 1; level < 4; level++) {
                while (!is_queue_empty(queues[level])) {
                    Process* p = dequeue(queues[level]);
                    enqueue(queues[0], p);
                }
            }
        }

        /* ВЫБОР ПРОЦЕССА ДЛЯ ВЫПОЛНЕНИЯ */

        /* Проверяем, есть ли процессы с более высоким приоритетом,
         * чем текущий выполняемый */
        if (s->running_process != NULL) {
            /* Определяем приоритет текущего процесса */
            int current_priority = s->running_process->priority;
            if (current_priority > 3) current_priority = 3;
            if (current_priority < 0) current_priority = 0;

            /* Ищем процесс с более высоким приоритетом (меньший номер) */
            bool higher_priority_found = false;
            for (int level = 0; level < current_priority; level++) {
                if (!is_queue_empty(queues[level])) {
                    higher_priority_found = true;
                    break;
                }
            }

            if (higher_priority_found) {
                /* ВЫТЕСНЕНИЕ: есть процесс с более высоким приоритетом */
                strncpy(s->running_process->status, "готов",
                    sizeof(s->running_process->status) - 1);
                s->running_process->status[sizeof(s->running_process->status) - 1] = '\0';

                /* Возвращаем текущий процесс в его очередь */
                enqueue(queues[current_priority], s->running_process);
                s->running_process = NULL;
            }
        }

        /* Если нет выполняющегося процесса, выбираем процесс с наивысшим приоритетом */
        if (s->running_process == NULL) {
            /* Ищем первую непустую очередь (с наивысшим приоритетом) */
            int selected_level = -1;
            for (int level = 0; level < 4; level++) {
                if (!is_queue_empty(queues[level])) {
                    selected_level = level;
                    break;
                }
            }

            if (selected_level != -1) {
                s->running_process = dequeue(queues[selected_level]);
                if (s->running_process->start_time == -1) {
                    s->running_process->start_time = s->current_time;
                }
                strncpy(s->running_process->status, "выполняется",
                    sizeof(s->running_process->status) - 1);
                s->running_process->status[sizeof(s->running_process->status) - 1] = '\0';
            }
        }

        /* ВЫПОЛНЕНИЕ ПРОЦЕССА */
        if (s->running_process != NULL) {
            s->running_process->remaining_time--;
            strncpy(s->running_process->status, "выполняется",
                sizeof(s->running_process->status) - 1);
            s->running_process->status[sizeof(s->running_process->status) - 1] = '\0';
            add_history(s, s->current_time, s->running_process->pid, "running");

            /* Проверяем завершение */
            if (s->running_process->remaining_time == 0) {
                s->running_process->completion_time = s->current_time + 1;
                s->running_process->turnaround_time =
                    s->running_process->completion_time - s->running_process->arrival_time;
                s->running_process->waiting_time =
                    s->running_process->turnaround_time - s->running_process->burst_time;
                strncpy(s->running_process->status, "завершен",
                    sizeof(s->running_process->status) - 1);
                s->running_process->status[sizeof(s->running_process->status) - 1] = '\0';

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
            add_history(s, s->current_time, -1, "idle");
        }

        s->current_time++;
    }

    /* Освобождаем очереди */
    for (int i = 0; i < 4; i++) {
        free_queue(queues[i]);
    }

    return s->completed;
}