#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "process.h"
#include "scheduler_core.h"
#include "fifo.h"
#include "round_robin.h"
#include "priority.h"
#include "visualizer.h"

/*
 * Тест 1: Базовый тест с тремя процессами
 * Проверяет работу всех алгоритмов на простом наборе процессов
 */
static void test_basic() {
    printf("%s%sТест 1: Базовый тест с тремя процессами%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);

    Process* test_procs[3];
    test_procs[0] = create_process(1, "P1", 0, 5, 1);
    test_procs[1] = create_process(2, "P2", 1, 3, 2);
    test_procs[2] = create_process(3, "P3", 2, 4, 0);

    if (!test_procs[0] || !test_procs[1] || !test_procs[2]) {
        printf("Ошибка создания тестовых процессов\n");
        return;
    }

    /* Тестируем FIFO */
    printf("\n--- FIFO ---\n");
    Scheduler* s = create_scheduler(test_procs, 3, 2);
    if (s) {
        fifo_scheduler(s);
        float avg = 0;
        for (int i = 0; i < s->num_completed; i++) {
            avg += s->completed[i]->waiting_time;
            printf("Процесс %s: ожидание=%d, оборот=%d\n",
                s->completed[i]->name,
                s->completed[i]->waiting_time,
                s->completed[i]->turnaround_time);
        }
        avg /= s->num_completed;
        printf("Среднее время ожидания: %.2f\n", avg);
        free_scheduler(s);
    }

    /* Тестируем Round Robin */
    printf("\n--- Round Robin (квант=2) ---\n");
    s = create_scheduler(test_procs, 3, 2);
    if (s) {
        round_robin_scheduler(s);
        float avg = 0;
        for (int i = 0; i < s->num_completed; i++) {
            avg += s->completed[i]->waiting_time;
            printf("Процесс %s: ожидание=%d, оборот=%d\n",
                s->completed[i]->name,
                s->completed[i]->waiting_time,
                s->completed[i]->turnaround_time);
        }
        avg /= s->num_completed;
        printf("Среднее время ожидания: %.2f\n", avg);
        free_scheduler(s);
    }

    /* Тестируем Priority */
    printf("\n--- Приоритетное планирование ---\n");
    s = create_scheduler(test_procs, 3, 2);
    if (s) {
        priority_scheduler(s);
        float avg = 0;
        for (int i = 0; i < s->num_completed; i++) {
            avg += s->completed[i]->waiting_time;
            printf("Процесс %s: ожидание=%d, оборот=%d\n",
                s->completed[i]->name,
                s->completed[i]->waiting_time,
                s->completed[i]->turnaround_time);
        }
        avg /= s->num_completed;
        printf("Среднее время ожидания: %.2f\n", avg);
        free_scheduler(s);
    }

    for (int i = 0; i < 3; i++) free_process(test_procs[i]);
}

/*
 * Тест 2: Проверка приоритетного вытеснения
 * Создает процессы с разными приоритетами и проверяет,
 * что высокоприоритетный процесс вытесняет низкоприоритетный
 */
static void test_priority_preemption() {
    printf("\n%s%sТест 2: Проверка приоритетного вытеснения%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);

    Process* test_procs[3];
    test_procs[0] = create_process(1, "LowPrio", 0, 8, 3);   /* Низкий приоритет */
    test_procs[1] = create_process(2, "HighPrio", 2, 3, 0);  /* Высокий приоритет */
    test_procs[2] = create_process(3, "MedPrio", 4, 2, 1);   /* Средний приоритет */

    if (!test_procs[0] || !test_procs[1] || !test_procs[2]) {
        printf("Ошибка создания тестовых процессов\n");
        return;
    }

    Scheduler* s = create_scheduler(test_procs, 3, 2);
    if (!s) {
        printf("Ошибка создания планировщика\n");
        return;
    }

    priority_scheduler(s);

    /* Проверяем, что HighPrio завершился раньше LowPrio */
    int high_complete = -1, low_complete = -1;
    for (int i = 0; i < s->num_completed; i++) {
        if (strcmp(s->completed[i]->name, "HighPrio") == 0)
            high_complete = s->completed[i]->completion_time;
        if (strcmp(s->completed[i]->name, "LowPrio") == 0)
            low_complete = s->completed[i]->completion_time;
    }

    printf("\nРезультаты:\n");
    for (int i = 0; i < s->num_completed; i++) {
        printf("Процесс %s: завершение=%d, приоритет=%d\n",
            s->completed[i]->name,
            s->completed[i]->completion_time,
            s->completed[i]->priority);
    }

    if (high_complete != -1 && low_complete != -1 && high_complete < low_complete) {
        printf("\n%sПриоритетное планирование работает корректно%s\n",
            COLOR_GREEN, COLOR_END);
    }
    else {
        printf("\n%sОшибка в приоритетном планировании%s\n",
            COLOR_RED, COLOR_END);
    }

    free_scheduler(s);
    for (int i = 0; i < 3; i++) free_process(test_procs[i]);
}

/*
 * Тест 3: Проверка квантования в Round Robin
 * Проверяет, что процессы правильно вытесняются по истечении кванта времени
 */
static void test_round_robin_quantum() {
    printf("\n%s%sТест 3: Проверка квантования в Round Robin%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);

    Process* test_procs[3];
    test_procs[0] = create_process(1, "P1", 0, 6, 0);
    test_procs[1] = create_process(2, "P2", 0, 4, 0);
    test_procs[2] = create_process(3, "P3", 0, 5, 0);

    if (!test_procs[0] || !test_procs[1] || !test_procs[2]) {
        printf("Ошибка создания тестовых процессов\n");
        return;
    }

    int quantum = 2;
    Scheduler* s = create_scheduler(test_procs, 3, quantum);
    if (!s) {
        printf("Ошибка создания планировщика\n");
        return;
    }

    round_robin_scheduler(s);

    printf("\nРезультаты с квантом %d:\n", quantum);
    for (int i = 0; i < s->num_completed; i++) {
        printf("Процесс %s: завершение=%d, ожидание=%d, оборот=%d\n",
            s->completed[i]->name,
            s->completed[i]->completion_time,
            s->completed[i]->waiting_time,
            s->completed[i]->turnaround_time);
    }

    if (s->num_completed == 3) {
        printf("\n%sВсе процессы завершены%s\n", COLOR_GREEN, COLOR_END);
    }
    else {
        printf("\n%sНе все процессы завершены%s\n", COLOR_RED, COLOR_END);
    }

    free_scheduler(s);
    for (int i = 0; i < 3; i++) free_process(test_procs[i]);
}

/*
 * Тест 4: Стресс-тест с большим количеством процессов
 * Проверяет работоспособность с 10 случайными процессами
 */
static void test_stress() {
    printf("\n%s%sТест 4: Стресс-тест с 10 процессами%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);

    int count = 10;
    Process** processes = generate_random_processes(&count);
    if (!processes) {
        printf("Ошибка генерации процессов\n");
        return;
    }

    printf("Сгенерировано %d процессов\n", count);

    Scheduler* s = create_scheduler(processes, count, 2);
    if (!s) {
        printf("Ошибка создания планировщика\n");
        free_processes(processes, count);
        return;
    }

    printf("\n--- FIFO ---\n");
    fifo_scheduler(s);
    float avg_wait = 0, avg_turnaround = 0;
    for (int i = 0; i < s->num_completed; i++) {
        avg_wait += s->completed[i]->waiting_time;
        avg_turnaround += s->completed[i]->turnaround_time;
    }
    avg_wait /= s->num_completed;
    avg_turnaround /= s->num_completed;
    printf("Ср. ожидание: %.2f, Ср. оборот: %.2f\n", avg_wait, avg_turnaround);

    free_scheduler(s);
    s = create_scheduler(processes, count, 2);

    printf("\n--- Round Robin ---\n");
    round_robin_scheduler(s);
    avg_wait = 0;
    avg_turnaround = 0;
    for (int i = 0; i < s->num_completed; i++) {
        avg_wait += s->completed[i]->waiting_time;
        avg_turnaround += s->completed[i]->turnaround_time;
    }
    avg_wait /= s->num_completed;
    avg_turnaround /= s->num_completed;
    printf("Ср. ожидание: %.2f, Ср. оборот: %.2f\n", avg_wait, avg_turnaround);

    free_scheduler(s);
    s = create_scheduler(processes, count, 2);

    printf("\n--- Priority ---\n");
    priority_scheduler(s);
    avg_wait = 0;
    avg_turnaround = 0;
    for (int i = 0; i < s->num_completed; i++) {
        avg_wait += s->completed[i]->waiting_time;
        avg_turnaround += s->completed[i]->turnaround_time;
    }
    avg_wait /= s->num_completed;
    avg_turnaround /= s->num_completed;
    printf("Ср. ожидание: %.2f, Ср. оборот: %.2f\n", avg_wait, avg_turnaround);

    free_scheduler(s);
    free_processes(processes, count);
    printf("\n%s✓ Стресс-тест пройден%s\n", COLOR_GREEN, COLOR_END);
}

/*
 * Функция: run_all_tests
 * Назначение: Запускает все автоматические тесты
 */
void run_all_tests() {
    clear_screen();
    printf("%s%s=== Запуск всех автоматических тестов ===%s\n\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);

    test_basic();
    test_priority_preemption();
    test_round_robin_quantum();
    test_stress();

    printf("\n%s%sВсе тесты завершены%s\n", COLOR_HEADER, COLOR_BOLD, COLOR_END);
    wait_for_enter();
}