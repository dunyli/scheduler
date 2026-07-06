#include "priority.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Структура очереди для приоритетного планирования */
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
 * Функции работы с очередью
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
 * Функция: priority_scheduler
 * Назначение: Запускает алгоритм приоритетного планирования
 * Параметры:
 *   s - указатель на планировщик
 * Возвращает: Массив завершенных процессов
 *
 * Алгоритм работы:
 * 1. Процессы распределяются по очередям в зависимости от приоритета
 * 2. Всегда выполняется процесс из очереди с наивысшим приоритетом
 * 3. При появлении процесса с более высоким приоритетом происходит вытеснение
 * 4. Каждые 20 тактов выполняется приоритетное ускорение (boost)
 * 5. Процессы перемещаются в очередь с наивысшим приоритетом
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
    int boost_counter = 0;  /* Счетчик для приоритетного ускорения */
    int current_priority = -1;

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
            enqueue(queues[priority], s->processes[process_index]);
            process_index++;
        }

        /*
         * Приоритетное ускорение (Priority Boosting)
         * Каждые 20 тактов все процессы перемещаются в очередь 0
         * Это предотвращает простой процессов с низким приоритетом
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

        /* Выбираем очередь с наивысшим приоритетом, в которой есть процессы */
        int selected_queue = -1;
        for (int level = 0; level < 4; level++) {
            if (!is_queue_empty(queues[level])) {
                selected_queue = level;
                break;
            }
        }

        /* Если нет выполняющегося процесса, берем из выбранной очереди */
        if (s->running_process == NULL && selected_queue != -1) {
            s->running_process = dequeue(queues[selected_queue]);
            if (s->running_process->start_time == -1) {
                s->running_process->start_time = s->current_time;
            }
            strcpy(s->running_process->status, "выполняется");
            current_priority = selected_queue;
        }

        /* Выполняем процесс */
        if (s->running_process != NULL) {
            s->running_process->remaining_time--;
            strcpy(s->running_process->status, "выполняется");
            add_history(s, s->current_time, s->running_process->pid, "running");

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
                current_priority = -1;
            }
            else if (selected_queue != -1) {
                /* Проверяем, не появился ли процесс с более высоким приоритетом */
                bool higher_priority = false;
                for (int level = 0; level < selected_queue; level++) {
                    if (!is_queue_empty(queues[level])) {
                        higher_priority = true;
                        break;
                    }
                }

                if (higher_priority) {
                    /* Вытесняем текущий процесс */
                    strcpy(s->running_process->status, "готов");
                    enqueue(queues[selected_queue], s->running_process);
                    s->running_process = NULL;
                    current_priority = -1;
                }
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