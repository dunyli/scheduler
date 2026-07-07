#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

/* Подключаем заголовочные файлы C */
#include "process.h"
#include "scheduler_core.h"
#include "fifo.h"
#include "round_robin.h"
#include "priority.h"
#include "visualizer.h"
#include "monitor.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#endif




/* Объявление функции из test.cpp */
void run_all_tests();


/*
 * Функция: run_interactive
 * Назначение: Запускает интерактивный режим работы
 * Позволяет пользователю выбирать алгоритмы и параметры
 */
static void run_interactive() {
    while (1) {
        clear_screen();
        printf("%s%s=== Симулятор планировщика процессов ===%s\n",
            COLOR_HEADER, COLOR_BOLD, COLOR_END);
        printf("\nВыберите алгоритм планирования:\n");
        printf("1. FIFO (First In First Out) - невытесняющий\n");
        printf("2. Round Robin (круговая очередь) - вытесняющий\n");
        printf("3. Приоритетное планирование (с очередями) - вытесняющий\n");
        printf("4. Сравнить все алгоритмы\n");
        printf("5. Выход\n");

        printf("\nВаш выбор (1-5): ");
        char choice[10];
        fgets(choice, sizeof(choice), stdin);
        int ch = atoi(choice);

        if (ch == 5) {
            printf("До свидания!\n");
            break;
        }

        if (ch < 1 || ch > 5) {
            printf("Неверный выбор. Нажмите Enter...");
            getchar();
            continue;
        }

        /* Запрос на использование тестовых процессов */
        printf("\nИспользовать тестовые процессы? (y/n): ");
        char use_test[10];
        fgets(use_test, sizeof(use_test), stdin);
        use_test[strcspn(use_test, "\n")] = 0;

        int num_processes;
        Process** processes;

        /* Генерация процессов */
        if (strcmp(use_test, "y") == 0 || strcmp(use_test, "Y") == 0) {
            processes = generate_test_processes(&num_processes);
        }
        else {
            processes = generate_random_processes(&num_processes);
        }

        if (!processes) {
            printf("Ошибка генерации процессов\n");
            continue;
        }

        if (ch == 4) {
            /* Сравнение всех алгоритмов */
            const char* names[] = { "FIFO", "Round Robin", "Priority" };
            Scheduler* schedulers[3];

            schedulers[0] = create_scheduler(processes, num_processes, 2);
            schedulers[1] = create_scheduler(processes, num_processes, 2);
            schedulers[2] = create_scheduler(processes, num_processes, 2);

            if (!schedulers[0] || !schedulers[1] || !schedulers[2]) {
                printf("Ошибка создания планировщиков\n");
                free_processes(processes, num_processes);
                for (int i = 0; i < 3; i++) {
                    if (schedulers[i]) free_scheduler(schedulers[i]);
                }
                continue;
            }

            /* Запуск всех алгоритмов */
            fifo_scheduler(schedulers[0]);
            round_robin_scheduler(schedulers[1]);
            priority_scheduler(schedulers[2]);

            /* Вывод результатов */
            for (int i = 0; i < 3; i++) {
                visualize_scheduler(schedulers[i], names[i]);
                printf("\n%s=== Нажмите Enter для продолжения ===%s\n",
                    COLOR_BOLD, COLOR_END);
                getchar();
                getchar();
            }

            for (int i = 0; i < 3; i++) {
                free_scheduler(schedulers[i]);
            }
        }
        else {
            /* Запуск выбранного алгоритма */
            Scheduler* s = create_scheduler(processes, num_processes, 2);
            const char* name;

            if (!s) {
                printf("Ошибка создания планировщика\n");
                free_processes(processes, num_processes);
                continue;
            }

            if (ch == 1) {
                fifo_scheduler(s);
                name = "FIFO";
            }
            else if (ch == 2) {
                printf("Введите квант времени (по умолчанию 2): ");
                char q_input[10];
                fgets(q_input, sizeof(q_input), stdin);
                int quantum = atoi(q_input);
                if (quantum <= 0) quantum = 2;

                free_scheduler(s);
                s = create_scheduler(processes, num_processes, quantum);
                if (!s) {
                    printf("Ошибка создания планировщика\n");
                    free_processes(processes, num_processes);
                    continue;
                }
                round_robin_scheduler(s);
                name = "Round Robin";
            }
            else {
                priority_scheduler(s);
                name = "Приоритетное планирование";
            }

            visualize_scheduler(s, name);
            free_scheduler(s);

            printf("\n%sНажмите Enter для продолжения...%s", COLOR_BOLD, COLOR_END);
            getchar();
            getchar();
        }

        free_processes(processes, num_processes);
    }
}

/*
 * Функция: main
 * Предоставляет выбор между интерактивным режимом и тестами
 */

int main() {
    /* Установка русской локали для корректного отображения */
    setlocale(LC_ALL, "Rus");

    printf("%s%sСИСТЕМНЫЙ МОНИТОР И ПЛАНИРОВЩИК%s\n", COLOR_HEADER, COLOR_BOLD, COLOR_END);
    printf("\n1. Мониторинг системы (реальные процессы)\n");
    printf("2. Симулятор планировщика\n");
    printf("3. Запустить тесты\n");
    printf("4. Выход\n");

    printf("\nВыберите режим (1-4): ");
    char mode[10];
    fgets(mode, sizeof(mode), stdin);
    int m = atoi(mode);

    switch (m) {
    case 1:
#ifdef _WIN32
        system_monitor_mode();
#else
        printf("Режим мониторинга только для Windows\n");
#endif
        break;
    case 2:
        run_interactive();
        break;
    case 3:
        run_all_tests();
        break;
    default:
        printf("До свидания!\n");
        break;
    }

    return 0;
}