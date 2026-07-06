#include "process.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/*
 * Функция: create_process
 * Назначение: Создает новый процесс и инициализирует все его поля
 * Параметры:
 *   pid - уникальный идентификатор процесса
 *   name - имя процесса (строка)
 *   arrival - время появления процесса в системе
 *   burst - общее время, необходимое для выполнения процесса
 *   priority - приоритет процесса (0 - наивысший, 3 - наинизший)
 * Возвращает: Указатель на созданный процесс или NULL в случае ошибки
 */
Process* create_process(int pid, const char* name, int arrival, int burst, int priority) {
    /* Выделяем память под структуру процесса */
    Process* p = (Process*)malloc(sizeof(Process));
    if (!p) {
        fprintf(stderr, "Ошибка: не удалось выделить память для процесса\n");
        return NULL;
    }

    /* Заполняем поля процесса */
    p->pid = pid;  /* Устанавливаем идентификатор */

    /* Копируем имя процесса с проверкой на переполнение буфера */
    strncpy(p->name, name, sizeof(p->name) - 1);
    p->name[sizeof(p->name) - 1] = '\0';  /* Гарантируем завершающий нуль */

    p->arrival_time = arrival;    /* Время прибытия */
    p->burst_time = burst;        /* Общее время выполнения */
    p->priority = priority;       /* Приоритет процесса */
    p->remaining_time = burst;    /* Изначально осталось столько же, сколько всего */
    p->start_time = -1;           /* -1 означает, что процесс еще не начинался */
    p->completion_time = -1;      /* -1 означает, что процесс еще не завершен */
    p->waiting_time = 0;          /* Изначально время ожидания равно 0 */
    p->turnaround_time = 0;       /* Изначально время оборота равно 0 */
    strcpy(p->status, "готов");   /* Начальный статус - "готов" */
    p->next = NULL;               /* Указатель на следующий процесс в очереди */

    return p;
}

/*
 * Функция: copy_process
 * Назначение: Копирует все данные из одного процесса в другой
 * Параметры:
 *   dest - указатель на процесс-приемник
 *   src - указатель на процесс-источник
 * Используется для создания копий процессов в планировщике
 */
void copy_process(Process* dest, const Process* src) {
    /* Проверка на нулевые указатели */
    if (!dest || !src) return;

    /* Копируем все поля из источника в приемник */
    dest->pid = src->pid;
    strncpy(dest->name, src->name, sizeof(dest->name) - 1);
    dest->name[sizeof(dest->name) - 1] = '\0';
    dest->arrival_time = src->arrival_time;
    dest->burst_time = src->burst_time;
    dest->priority = src->priority;
    dest->remaining_time = src->remaining_time;
    dest->start_time = src->start_time;
    dest->completion_time = src->completion_time;
    dest->waiting_time = src->waiting_time;
    dest->turnaround_time = src->turnaround_time;
    strcpy(dest->status, src->status);
    dest->next = NULL;  /* Обнуляем указатель на следующий элемент */
}

/*
 * Функция: free_process
 * Назначение: Освобождает память, занятую процессом
 * Параметры:
 *   p - указатель на процесс для освобождения
 */
void free_process(Process* p) {
    if (p) free(p);  /* Освобождаем память только если указатель не NULL */
}

/*
 * Функция: print_process
 * Назначение: Выводит информацию о процессе в консоль
 * Параметры:
 *   p - указатель на процесс для вывода
 * Возвращает: ничего (void)
 * Примечание: Используется для отладки и тестирования
 */
void print_process(const Process* p) {
    if (!p) return;  /* Проверка на нулевой указатель */
    printf("PID: %d, Имя: %s, Прибытие: %d, Выполнение: %d, Приоритет: %d, Статус: %s\n",
        p->pid, p->name, p->arrival_time, p->burst_time, p->priority, p->status);
}

/*
 * Функция: compare_arrival_time
 * Назначение: Сравнивает два процесса по времени прибытия
 * Параметры:
 *   a - первый процесс для сравнения
 *   b - второй процесс для сравнения
 * Возвращает: true, если a прибыл раньше b, иначе false
 * Используется для сортировки процессов по времени прибытия
 */
bool compare_arrival_time(const Process* a, const Process* b) {
    if (!a || !b) return false;  /* Защита от нулевых указателей */
    return a->arrival_time < b->arrival_time;  /* Сравнение времени прибытия */
}

/*
 * Функция: generate_test_processes
 * Назначение: Генерирует набор тестовых процессов с заданными параметрами
 * Параметры:
 *   count - указатель на переменную, куда будет записано количество процессов
 * Возвращает: Массив указателей на процессы или NULL в случае ошибки
 * Создает 6 процессов с различными временами прибытия,
 *             приоритетами и временами выполнения для тестирования
 */
Process** generate_test_processes(int* count) {
    *count = 6;  /* Фиксированное количество процессов для теста */

    /* Выделяем память под массив указателей на процессы */
    Process** processes = (Process**)malloc(*count * sizeof(Process*));
    if (!processes) {
        fprintf(stderr, "Ошибка: не удалось выделить память для процессов\n");
        return NULL;
    }

    /* Предопределенные параметры для тестовых процессов */
    const char* names[] = { "Firefox", "Chrome", "Python", "Java", "Docker", "MySQL" };
    int priorities[] = { 1, 0, 2, 1, 3, 0 };  /* Приоритеты (0 - наивысший) */
    int arrivals[] = { 0, 1, 2, 3, 5, 7 };    /* Время прибытия */
    int bursts[] = { 5, 3, 4, 6, 2, 4 };      /* Время выполнения */

    /* Создаем каждый процесс и добавляем в массив */
    for (int i = 0; i < *count; i++) {
        processes[i] = create_process(i + 1, names[i], arrivals[i], bursts[i], priorities[i]);
        if (!processes[i]) {
            /* В случае ошибки освобождаем уже созданные процессы */
            for (int j = 0; j < i; j++) free_process(processes[j]);
            free(processes);
            return NULL;
        }
    }
    return processes;
}

/*
 * Функция: generate_random_processes
 * Назначение: Генерирует случайный набор процессов по запросу пользователя
 * Параметры:
 *   count - указатель на переменную, куда будет записано количество процессов
 * Возвращает: Массив указателей на процессы или NULL в случае ошибки
 * Пользователь вводит количество процессов (по умолчанию 5),
 *             все параметры генерируются случайно
 */
Process** generate_random_processes(int* count) {
    /* Запрос количества процессов у пользователя */
    printf("Введите количество процессов (по умолчанию 5): ");
    char input[10];
    fgets(input, sizeof(input), stdin);
    *count = atoi(input);

    /* Проверка и коррекция введенного значения */
    if (*count <= 0) *count = 5;   /* Если введено 0 или меньше - ставим 5 */
    if (*count > 10) *count = 10;  /* Ограничиваем максимум 10 процессами */

    /* Выделяем память под массив указателей на процессы */
    Process** processes = (Process**)malloc(*count * sizeof(Process*));
    if (!processes) {
        fprintf(stderr, "Ошибка: не удалось выделить память для процессов\n");
        return NULL;
    }

    /* Массив имен для случайного выбора */
    const char* names[] = { "Firefox", "Chrome", "Python", "Java", "Docker",
                           "MySQL", "Node", "Redis", "Nginx", "GCC" };

    /* Инициализация генератора случайных чисел */
    srand(time(NULL));

    /* Создаем процессы со случайными параметрами */
    for (int i = 0; i < *count; i++) {
        int name_idx = rand() % 10;  /* Случайный индекс имени */
        char name[20];
        sprintf(name, "%s%d", names[name_idx], i + 1);  /* Формируем имя с номером */

        /* Случайные параметры процесса:
         * - прибытие от 0 до 9
         * - выполнение от 2 до 7 (rand() % 6 + 2)
         * - приоритет от 0 до 3
         */
        processes[i] = create_process(i + 1, name,
            rand() % 10,     /* Время прибытия */
            rand() % 6 + 2,  /* Время выполнения */
            rand() % 4);     /* Приоритет */
        if (!processes[i]) {
            /* В случае ошибки освобождаем уже созданные процессы */
            for (int j = 0; j < i; j++) free_process(processes[j]);
            free(processes);
            return NULL;
        }
    }
    return processes;
}

/*
 * Функция: free_processes
 * Назначение: Освобождает память, занятую массивом процессов
 * Параметры:
 *   processes - массив указателей на процессы
 *   count - количество процессов в массиве
 * Возвращает: ничего (void)
 * Последовательно освобождает каждый процесс и сам массив
 */
void free_processes(Process** processes, int count) {
    if (!processes) return;  /* Проверка на нулевой указатель */

    /* Освобождаем каждый процесс в массиве */
    for (int i = 0; i < count; i++) free_process(processes[i]);

    /* Освобождаем сам массив указателей */
    free(processes);
}