#define _CRT_SECURE_NO_WARNINGS
#include "visualizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Функция: clear_screen
 * Назначение: Очищает экран терминала
 */
void clear_screen() {
    printf("\033[2J\033[1;1H");
}

/*
 * Функция: get_status_color
 * Назначение: Возвращает цвет для статуса процесса
 */
static const char* get_status_color(const char* status) {
    if (strcmp(status, "завершен") == 0) return COLOR_GREEN;
    if (strcmp(status, "выполняется") == 0) return COLOR_YELLOW;
    if (strcmp(status, "готов") == 0) return COLOR_CYAN;
    return COLOR_END;
}

/*
 * Функция: display_process_info
 * Назначение: Выводит таблицу с информацией о процессах
 */
void display_process_info(Scheduler* s) {
    if (!s) return;

    printf("%s%s=== Информация о процессах ===%s\n", COLOR_HEADER, COLOR_BOLD, COLOR_END);
    printf("%-6s %-10s %-10s %-10s %-10s %-12s\n",
        "PID", "Имя", "Прибытие", "Выполнение", "Приоритет", "Статус");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < s->num_processes; i++) {
        Process* p = s->processes[i];
        const char* color = get_status_color(p->status);
        printf("%-6d %-10s %-10d %-10d %-10d %s%s%s\n",
            p->pid, p->name, p->arrival_time, p->burst_time,
            p->priority, color, p->status, COLOR_END);
    }
    printf("\n");
}

/*
 * Функция: display_schedule_table
 * Назначение: Отображает таблицу с временными интервалами выполнения
 * Вместо диаграммы Ганта показывает:
 * - Время начала интервала
 * - Какой процесс выполняется
 * - Длительность интервала
 */
static void display_schedule_table(Scheduler* s) {
    if (!s || s->history_size == 0) {
        printf("Нет данных для отображения.\n");
        return;
    }

    printf("%s%s=== РАСПИСАНИЕ ВЫПОЛНЕНИЯ (ПО ИНТЕРВАЛАМ) ===%s\n",
        COLOR_BOLD, COLOR_CYAN, COLOR_END);
    printf("\n");
    printf("%-15s %-25s %-15s %-15s\n",
        "Начало", "Процесс", "Длительность", "До");
    printf("--------------------------------------------------------------------------------\n");

    int start_time = s->history[0].time;
    int current_pid = s->history[0].pid;
    int total_idle = 0;
    int total_busy = 0;

    for (int i = 1; i < s->history_size; i++) {
        if (s->history[i].pid != current_pid) {
            int duration = s->history[i].time - start_time;
            int end_time = s->history[i].time;

            if (current_pid == -1) {
                /* Простой процессора */
                printf("%-15d %-25s %-15d %-15d\n",
                    start_time, "ПРОСТОЙ (IDLE)", duration, end_time);
                total_idle += duration;
            }
            else {
                /* Выполнение процесса */
                Process* p = get_process_by_pid(s, current_pid);
                if (p) {
                    char display_name[30];
                    snprintf(display_name, sizeof(display_name), "%s (PID=%d)", p->name, p->pid);
                    printf("%-15d %-25s %-15d %-15d\n",
                        start_time, display_name, duration, end_time);
                    total_busy += duration;
                }
            }

            start_time = s->history[i].time;
            current_pid = s->history[i].pid;
        }
    }

    /* Последний интервал */
    int duration = s->current_time - start_time;
    int end_time = s->current_time;

    if (current_pid == -1) {
        printf("%-15d %-25s %-15d %-15d\n",
            start_time, "ПРОСТОЙ (IDLE)", duration, end_time);
        total_idle += duration;
    }
    else {
        Process* p = get_process_by_pid(s, current_pid);
        if (p) {
            char display_name[30];
            snprintf(display_name, sizeof(display_name), "%s (PID=%d)", p->name, p->pid);
            printf("%-15d %-25s %-15d %-15d\n",
                start_time, display_name, duration, end_time);
            total_busy += duration;
        }
    }

    printf("--------------------------------------------------------------------------------\n");

    /* Итоговая статистика по времени */
    printf("\n%sСводка по времени:%s\n", COLOR_BOLD, COLOR_END);
    printf("  Всего тактов: %d\n", s->current_time);
    printf("  Время выполнения процессов: %d (%.1f%%)\n",
        total_busy, (float)total_busy / s->current_time * 100);
    printf("  Время простоя: %d (%.1f%%)\n",
        total_idle, (float)total_idle / s->current_time * 100);
    printf("  Количество переключений: %d\n", s->history_size - 1);

    /* Подробная информация по каждому процессу */
    printf("\n%sВремя выполнения по процессам:%s\n", COLOR_BOLD, COLOR_END);

    /* Собираем статистику по каждому процессу */
    int* process_time = (int*)calloc(s->num_processes, sizeof(int));
    if (process_time) {
        start_time = s->history[0].time;
        current_pid = s->history[0].pid;

        for (int i = 1; i < s->history_size; i++) {
            if (s->history[i].pid != current_pid) {
                duration = s->history[i].time - start_time;
                if (current_pid != -1) {
                    for (int j = 0; j < s->num_processes; j++) {
                        if (s->processes[j]->pid == current_pid) {
                            process_time[j] += duration;
                            break;
                        }
                    }
                }
                start_time = s->history[i].time;
                current_pid = s->history[i].pid;
            }
        }

        /* Последний интервал */
        duration = s->current_time - start_time;
        if (current_pid != -1) {
            for (int j = 0; j < s->num_processes; j++) {
                if (s->processes[j]->pid == current_pid) {
                    process_time[j] += duration;
                    break;
                }
            }
        }

        /* Вывод статистики по каждому процессу */
        for (int i = 0; i < s->num_processes; i++) {
            Process* p = s->processes[i];
            int time = process_time[i];
            printf("  %s: %d тактов (%.1f%%)\n",
                p->name, time, (float)time / s->current_time * 100);
        }

        free(process_time);
    }

    printf("\n");
}

/*
 * Функция: display_statistics
 * Назначение: Отображает статистику выполнения
 */
void display_statistics(Scheduler* s) {
    if (!s || s->num_completed == 0) {
        printf("Нет завершенных процессов.\n");
        return;
    }

    printf("\n%s%s=== СТАТИСТИКА ПРОЦЕССОВ ===%s\n", COLOR_BOLD, COLOR_HEADER, COLOR_END);

    int total_wait = 0, total_turnaround = 0;
    for (int i = 0; i < s->num_completed; i++) {
        total_wait += s->completed[i]->waiting_time;
        total_turnaround += s->completed[i]->turnaround_time;
    }

    float avg_wait = (float)total_wait / s->num_completed;
    float avg_turnaround = (float)total_turnaround / s->num_completed;

    printf("\n%-10s %-15s %-15s %-15s %-15s\n",
        "Процесс", "Прибытие", "Выполнение", "Ожидание", "Оборот");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < s->num_completed; i++) {
        Process* p = s->completed[i];
        printf("%-10s %-15d %-15d %-15d %-15d\n",
            p->name, p->arrival_time, p->burst_time,
            p->waiting_time, p->turnaround_time);
    }

    printf("------------------------------------------------------------\n");
    printf("\n%sОбщая статистика:%s\n", COLOR_BOLD, COLOR_END);
    printf("  Среднее время ожидания: %.2f\n", avg_wait);
    printf("  Среднее время оборота: %.2f\n", avg_turnaround);
    printf("  Пропускная способность: %d процессов\n", s->num_completed);

    /* Минимальное и максимальное время */
    if (s->num_completed > 0) {
        int min_wait = s->completed[0]->waiting_time;
        int max_wait = s->completed[0]->waiting_time;
        int min_turnaround = s->completed[0]->turnaround_time;
        int max_turnaround = s->completed[0]->turnaround_time;

        for (int i = 1; i < s->num_completed; i++) {
            if (s->completed[i]->waiting_time < min_wait)
                min_wait = s->completed[i]->waiting_time;
            if (s->completed[i]->waiting_time > max_wait)
                max_wait = s->completed[i]->waiting_time;
            if (s->completed[i]->turnaround_time < min_turnaround)
                min_turnaround = s->completed[i]->turnaround_time;
            if (s->completed[i]->turnaround_time > max_turnaround)
                max_turnaround = s->completed[i]->turnaround_time;
        }

        printf("\n%sЭкстремумы:%s\n", COLOR_BOLD, COLOR_END);
        printf("  Минимальное время ожидания: %d\n", min_wait);
        printf("  Максимальное время ожидания: %d\n", max_wait);
        printf("  Минимальное время оборота: %d\n", min_turnaround);
        printf("  Максимальное время оборота: %d\n", max_turnaround);
    }
}

/*
 * Функция: visualize_scheduler
 * Назначение: Полная визуализация работы планировщика
 */
void visualize_scheduler(Scheduler* s, const char* algorithm_name) {
    clear_screen();

    printf("%s%s=== СИМУЛЯТОР ПЛАНИРОВЩИКА ПРОЦЕССОВ ===%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);
    printf("%sАлгоритм: %s%s\n", COLOR_BOLD, algorithm_name, COLOR_END);
    printf("Квант времени: %d\n\n", s->time_quantum);

    display_process_info(s);
    display_schedule_table(s);    /* Вместо диаграммы Ганта */
    display_statistics(s);
}

/*
 * Функция: wait_for_enter
 * Назначение: Ожидает нажатия клавиши Enter
 */
void wait_for_enter() {
    printf("\n%sНажмите Enter для продолжения...%s", COLOR_BOLD, COLOR_END);
    getchar();
    getchar();
}