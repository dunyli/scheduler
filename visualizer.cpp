#include "visualizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Функция: clear_screen
 * Назначение: Очищает экран терминала
 * Использует ANSI escape-последовательности
 */
void clear_screen() {
    printf("\033[2J\033[1;1H");
}

/*
 * Функция: get_status_color
 * Назначение: Возвращает цвет для статуса процесса
 * Параметры:
 *   status - строка со статусом
 * Возвращает: ANSI-код цвета
 */
static const char* get_status_color(const char* status) {
    if (strcmp(status, "завершен") == 0) return COLOR_GREEN;
    if (strcmp(status, "выполняется") == 0) return COLOR_YELLOW;
    if (strcmp(status, "готов") == 0) return COLOR_CYAN;
    return COLOR_END;
}

/*
 * Функция: display_process_info
 * Назначение: Выводит таблицу с информацией о всех процессах
 * Параметры:
 *   s - указатель на планировщик
 */
void display_process_info(Scheduler* s) {
    if (!s) return;

    printf("%s%s=== Информация о процессах ===%s\n", COLOR_HEADER, COLOR_BOLD, COLOR_END);
    printf("%-6s %-10s %-10s %-10s %-10s %-12s\n",
        "PID", "Имя", "Прибытие", "Выполнение", "Приоритет", "Статус");
    printf("----------------------------------------------------------------------\n");

    /* Выводим информацию о каждом процессе */
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
 * Функция: display_gantt_chart
 * Назначение: Отображает диаграмму Ганта в терминале
 * Параметры:
 *   s - указатель на планировщик
 * Сжимает историю для компактного отображения
 */
void display_gantt_chart(Scheduler* s) {
    if (!s || s->history_size == 0) {
        printf("Нет данных для отображения.\n");
        return;
    }

    printf("%s%sДиаграмма Ганта (время 0-%d):%s\n",
        COLOR_BOLD, COLOR_CYAN, s->current_time, COLOR_END);

    /* Структура для сжатой записи */
    typedef struct GanttEntry {
        int start;
        int end;
        int pid;
    } GanttEntry;

    GanttEntry* gantt = (GanttEntry*)malloc(s->history_size * sizeof(GanttEntry));
    if (!gantt) {
        printf("Ошибка выделения памяти\n");
        return;
    }

    /* Сжимаем последовательные записи с одинаковым PID */
    int gantt_size = 0;
    int current_pid = s->history[0].pid;
    int start_time = s->history[0].time;

    for (int i = 1; i < s->history_size; i++) {
        if (s->history[i].pid != current_pid) {
            gantt[gantt_size].start = start_time;
            gantt[gantt_size].end = s->history[i].time;
            gantt[gantt_size].pid = current_pid;
            gantt_size++;
            current_pid = s->history[i].pid;
            start_time = s->history[i].time;
        }
    }
    /* Добавляем последнюю запись */
    gantt[gantt_size].start = start_time;
    gantt[gantt_size].end = s->current_time;
    gantt[gantt_size].pid = current_pid;
    gantt_size++;

    /* Отображаем диаграмму */
    int max_width = 80;  /* Максимальная ширина диаграммы */
    int total_time = s->current_time;
    int scale = total_time / max_width + 1;  /* Масштаб для сжатия */

    /* Цвета для разных процессов */
    const char* colors[] = { COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
                            COLOR_CYAN, COLOR_MAGENTA, COLOR_HEADER, "" };

    /* Выводим каждый сегмент диаграммы */
    for (int i = 0; i < gantt_size && i < max_width; i++) {
        int length = gantt[i].end - gantt[i].start;
        char symbol = '.';
        const char* color = COLOR_END;

        if (gantt[i].pid != -1) {
            /* Находим имя процесса по PID */
            for (int j = 0; j < s->num_processes; j++) {
                if (s->processes[j]->pid == gantt[i].pid) {
                    symbol = s->processes[j]->name[0];  /* Первая буква имени */
                    color = colors[gantt[i].pid % 7];
                    break;
                }
            }
        }

        int count = length / scale;
        if (count < 1) count = 1;
        for (int j = 0; j < count; j++) {
            if (gantt[i].pid == -1) {
                printf(".");  /* Простой процессора */
            }
            else {
                printf("%s%c%s", color, symbol, COLOR_END);
            }
        }
    }
    printf("\n");

    /* Метки времени */
    printf(" ");
    for (int i = 0; i <= total_time; i += total_time / 20 + 1) {
        printf("%-4d", i);
    }
    printf("\n");

    free(gantt);
}

/*
 * Функция: display_statistics
 * Назначение: Отображает статистику выполнения процессов
 * Параметры:
 *   s - указатель на планировщик
 */
void display_statistics(Scheduler* s) {
    if (!s || s->num_completed == 0) {
        printf("Нет завершенных процессов.\n");
        return;
    }

    printf("\n%s%sСтатистика:%s\n", COLOR_BOLD, COLOR_HEADER, COLOR_END);

    /* Вычисляем средние значения */
    int total_wait = 0, total_turnaround = 0;
    for (int i = 0; i < s->num_completed; i++) {
        total_wait += s->completed[i]->waiting_time;
        total_turnaround += s->completed[i]->turnaround_time;
    }

    float avg_wait = (float)total_wait / s->num_completed;
    float avg_turnaround = (float)total_turnaround / s->num_completed;

    printf("Среднее время ожидания: %.2f\n", avg_wait);
    printf("Среднее время оборота: %.2f\n", avg_turnaround);
    printf("Пропускная способность: %d процессов\n", s->num_completed);

    /* Детальная информация по каждому процессу */
    printf("\n%sДетальная информация:%s\n", COLOR_BOLD, COLOR_END);
    printf("%-6s %-10s %-10s %-10s %-10s\n",
        "PID", "Имя", "Ожидание", "Оборота", "Завершение");
    printf("------------------------------------------------------------\n");

    for (int i = 0; i < s->num_completed; i++) {
        Process* p = s->completed[i];
        printf("%-6d %-10s %-10d %-10d %-10d\n",
            p->pid, p->name, p->waiting_time,
            p->turnaround_time, p->completion_time);
    }
}

/*
 * Функция: visualize_scheduler
 * Назначение: Полная визуализация работы планировщика
 * Параметры:
 *   s - указатель на планировщик
 *   algorithm_name - название алгоритма
 */
void visualize_scheduler(Scheduler* s, const char* algorithm_name) {
    clear_screen();

    printf("%s%s=== Симулятор планировщика процессов ===%s\n",
        COLOR_HEADER, COLOR_BOLD, COLOR_END);
    printf("%sАлгоритм: %s%s\n", COLOR_BOLD, algorithm_name, COLOR_END);
    printf("Квант времени: %d\n\n", s->time_quantum);

    display_process_info(s);
    display_gantt_chart(s);
    display_statistics(s);
}

/*
 * Функция: wait_for_enter
 * Назначение: Ожидает нажатия клавиши Enter
 * Используется для паузы между выводами
 */
void wait_for_enter() {
    printf("\n%sНажмите Enter для продолжения...%s", COLOR_BOLD, COLOR_END);
    getchar();
    getchar();  /* Двойной вызов для очистки буфера */
}