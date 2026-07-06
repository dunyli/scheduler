#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "scheduler_core.h"

/* ANSI цвета для терминала */
#define COLOR_HEADER   "\033[95m"
#define COLOR_BLUE     "\033[94m"
#define COLOR_CYAN     "\033[96m"
#define COLOR_GREEN    "\033[92m"
#define COLOR_YELLOW   "\033[93m"
#define COLOR_RED      "\033[91m"
#define COLOR_MAGENTA  "\033[35m"
#define COLOR_END      "\033[0m"
#define COLOR_BOLD     "\033[1m"
#define COLOR_UNDERLINE "\033[4m"

/* Очищает экран */
void clear_screen();

/* Отображает информацию о процессах */
void display_process_info(Scheduler* s);

/* Отображает диаграмму Ганта */
void display_gantt_chart(Scheduler* s);

/* Отображает статистику */
void display_statistics(Scheduler* s);

/* Полная визуализация */
void visualize_scheduler(Scheduler* s, const char* algorithm_name);

/* Ждет нажатия Enter */
void wait_for_enter();

#endif 