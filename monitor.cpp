#define _CRT_SECURE_NO_WARNINGS
#include "monitor.h"
#include "visualizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>      // Для _kbhit() и _getch() - проверка нажатия клавиш

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

// Структура для хранения информации о процессе (для сортировки)
typedef struct {
    DWORD pid;              // Идентификатор процесса
    char name[256];         // Имя процесса
    DWORD memory;           // Использование памяти в KB
    DWORD priority;         // Приоритет процесса
} ProcessInfo;

/*
 * Функция: compare_memory
 * Назначение: Функция сравнения для сортировки процессов по памяти
 * Параметры:
 *   a - указатель на первый процесс
 *   b - указатель на второй процесс
 * Возвращает:
 *   положительное число, если a > b (по убыванию)
 *   отрицательное, если a < b
 *   0, если равны
 *
 * Используется в qsort() для сортировки процессов
 * Сортировка по убыванию - самые тяжелые процессы сверху
 */
static int compare_memory(const void* a, const void* b) {
    ProcessInfo* pa = (ProcessInfo*)a;
    ProcessInfo* pb = (ProcessInfo*)b;
    // Сортируем по убыванию (самые тяжелые сверху)
    return pb->memory - pa->memory;
}

/*
 * Функция: show_cpu_memory
 * Назначение: Отображает текущую загрузку CPU и использование памяти
 *
 * Алгоритм работы:
 * 1. Для памяти: использует GlobalMemoryStatusEx() для получения информации
 * 2. Для CPU: использует GetSystemTimes() и вычисляет разницу между замерами
 * 3. static переменные сохраняют предыдущие значения для расчета процента загрузки
 *
 * Цветовая индикация:
 * - Зеленый: нормальная загрузка (< 60% памяти, < 50% CPU)
 * - Желтый: средняя загрузка (60-80% памяти, 50-80% CPU)
 * - Красный: высокая загрузка (> 80% памяти, > 80% CPU)
 */
void show_cpu_memory(void) {
    // === ИНФОРМАЦИЯ О ПАМЯТИ ===
    // MEMORYSTATUSEX - структура для хранения информации о памяти
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);  // Заполняем структуру данными

    // Вычисляем процент использования памяти
    // ullTotalPhys - общая физическая память
    // ullAvailPhys - доступная память
    float mem_percent = (float)(memInfo.ullTotalPhys - memInfo.ullAvailPhys) /
        memInfo.ullTotalPhys * 100;

    // Выбираем цвет для индикации загрузки памяти
    const char* mem_color = COLOR_GREEN;      // По умолчанию зеленый
    if (mem_percent > 80) mem_color = COLOR_RED;      // Критично - красный
    else if (mem_percent > 60) mem_color = COLOR_YELLOW; // Средне - желтый

    // Выводим информацию о памяти с цветом
    printf("%sПамять:%s %s%.1f%%%s использовано (%llu MB / %llu MB)\n",
        COLOR_BOLD, COLOR_END,
        mem_color, mem_percent, COLOR_END,
        (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024 * 1024),  // Использовано MB
        memInfo.ullTotalPhys / (1024 * 1024));                          // Всего MB

    // === ИНФОРМАЦИЯ О CPU ===
    // static переменные сохраняют значение между вызовами функции
    // Это нужно для вычисления разницы во времени
    static ULONGLONG last_idle = 0, last_kernel = 0, last_user = 0;

    // Получаем время работы системы в разных режимах
    // idleTime - время простоя процессора
    // kernelTime - время выполнения ядра
    // userTime - время выполнения пользовательских процессов
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    // Конвертируем FILETIME в 64-битное число для вычислений
    // dwHighDateTime и dwLowDateTime - две части 64-битного значения
    ULONGLONG idle = ((ULONGLONG)idleTime.dwHighDateTime << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = ((ULONGLONG)kernelTime.dwHighDateTime << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = ((ULONGLONG)userTime.dwHighDateTime << 32) | userTime.dwLowDateTime;

    // При первом вызове last_idle = 0, поэтому пропускаем вычисление
    // Нужно два замера для вычисления процента загрузки
    if (last_idle != 0) {
        // Вычисляем разницу между замерами
        ULONGLONG idle_diff = idle - last_idle;        // Сколько времени CPU простаивал
        ULONGLONG total_diff = (kernel + user) - (last_kernel + last_user);  // Общее время работы

        // Процент загрузки = 100% - процент простоя
        float cpu_percent = 100.0f - (float)idle_diff / total_diff * 100.0f;

        // Выбираем цвет для индикации загрузки CPU
        const char* cpu_color = COLOR_GREEN;      // По умолчанию зеленый
        if (cpu_percent > 80) cpu_color = COLOR_RED;      // Критично - красный
        else if (cpu_percent > 50) cpu_color = COLOR_YELLOW; // Средне - желтый

        // Выводим информацию о CPU с цветом
        printf("%sCPU:%s %s%.1f%%%s\n", COLOR_BOLD, COLOR_END, cpu_color, cpu_percent, COLOR_END);
    }

    // Сохраняем текущие значения для следующего вызова
    last_idle = idle;
    last_kernel = kernel;
    last_user = user;
}

/*
 * Функция: show_real_processes
 * Назначение: Отображает список реальных процессов в системе (до 50)
 *
 * Алгоритм работы:
 * 1. Создает снимок (snapshot) всех процессов через CreateToolhelp32Snapshot()
 * 2. Для каждого процесса получает:
 *    - PID (идентификатор процесса)
 *    - Имя процесса (через модули)
 *    - Использование памяти
 *    - Приоритет выполнения
 * 3. Сохраняет информацию в массив
 * 4. Сортирует процессы по использованию памяти (самые тяжелые сверху)
 * 5. Выводит первые 50 процессов в виде таблицы
 *
 * Цветовая индикация по памяти:
 * - Зеленый: < 50 MB
 * - Желтый: 50-100 MB
 * - Красный: > 100 MB
 */
void show_real_processes(void) {
    // Создаем снимок всех процессов в системе
    // TH32CS_SNAPPROCESS - флаг, указывающий что нужны процессы
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printf("Ошибка: не удалось получить список процессов\n");
        return;
    }

    // Массив для хранения информации о процессах (максимум 500)
    ProcessInfo processes[500];
    int process_count = 0;

    // PROCESSENTRY32W - структура для хранения информации о процессе (Unicode версия)
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    // Process32FirstW - получаем первый процесс из снимка (Unicode версия)
    if (Process32FirstW(snapshot, &pe)) {
        do {
            // Открываем процесс для получения дополнительной информации
            HANDLE hProcess = OpenProcess(
                PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,  // Запрашиваемые права
                FALSE,  // Не наследовать дескриптор
                pe.th32ProcessID);  // PID процесса

            char process_name[256] = "";  // Буфер для имени процесса
            DWORD mem_usage = 0;           // Использование памяти в KB
            DWORD priority = 0;            // Приоритет процесса

            if (hProcess) {
                // === ПОЛУЧАЕМ ИМЯ ПРОЦЕССА ===
                // Через модули процесса (более надежный способ)
                HMODULE hMods[1024];
                DWORD cbNeeded;

                if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
                    WCHAR mod_name[MAX_PATH];  // Буфер для имени модуля (Unicode)
                    // GetModuleFileNameExW - получаем полный путь к исполняемому файлу
                    if (GetModuleFileNameExW(hProcess, hMods[0], mod_name, MAX_PATH)) {
                        // Извлекаем только имя файла из полного пути
                        WCHAR* last_slash = wcsrchr(mod_name, L'\\');
                        if (last_slash) {
                            // Конвертируем WCHAR (Unicode) в char (ANSI)
                            wcstombs(process_name, last_slash + 1, sizeof(process_name) - 1);
                        }
                        else {
                            wcstombs(process_name, mod_name, sizeof(process_name) - 1);
                        }
                        process_name[sizeof(process_name) - 1] = '\0';
                    }
                }

                // Если не удалось получить имя через модули - используем имя из PROCESSENTRY32
                if (strlen(process_name) == 0) {
                    wcstombs(process_name, pe.szExeFile, sizeof(process_name) - 1);
                    process_name[sizeof(process_name) - 1] = '\0';
                }

                // === ПОЛУЧАЕМ ИСПОЛЬЗОВАНИЕ ПАМЯТИ ===
                // PROCESS_MEMORY_COUNTERS - структура для информации о памяти процесса
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    // WorkingSetSize - объем физической памяти, используемый процессом
                    mem_usage = pmc.WorkingSetSize / 1024;  // Переводим в KB
                }

                // === ПОЛУЧАЕМ ПРИОРИТЕТ ПРОЦЕССА ===
                priority = GetPriorityClass(hProcess);

                CloseHandle(hProcess);  // Закрываем дескриптор процесса
            }
            else {
                // Если не удалось открыть процесс - используем имя из PROCESSENTRY32
                wcstombs(process_name, pe.szExeFile, sizeof(process_name) - 1);
                process_name[sizeof(process_name) - 1] = '\0';
            }

            // Очищаем имя от непечатных символов (мусора)
            for (int i = 0; i < strlen(process_name); i++) {
                if (process_name[i] < 32 || process_name[i] > 126) {
                    process_name[i] = ' ';
                }
            }

            // Сохраняем информацию о процессе в массив
            if (process_count < 500) {
                processes[process_count].pid = pe.th32ProcessID;
                strncpy(processes[process_count].name, process_name, sizeof(processes[process_count].name) - 1);
                processes[process_count].name[sizeof(processes[process_count].name) - 1] = '\0';
                processes[process_count].memory = mem_usage;
                processes[process_count].priority = priority;
                process_count++;
            }

        } while (Process32NextW(snapshot, &pe));  // Переходим к следующему процессу
    }

    CloseHandle(snapshot);  // Закрываем снимок (освобождаем ресурсы)

    // === СОРТИРОВКА ПРОЦЕССОВ ===
    // Сортируем по использованию памяти (самые тяжелые сверху)
    qsort(processes, process_count, sizeof(ProcessInfo), compare_memory);

    // === ВЫВОД ТАБЛИЦЫ ===
    // Выводим заголовок таблицы
    printf("\n%s=== ТОП ПРОЦЕССОВ (по использованию памяти) ===%s\n", COLOR_HEADER, COLOR_END);
    printf("%-8s %-45s %-15s %-10s %-8s\n",
        "PID", "Имя процесса", "Память, KB", "Приоритет", "Админ");
    printf("--------------------------------------------------------------------------------------------------------\n");

    // Определяем сколько процессов выводить (максимум 50)
    int display_count = process_count < 50 ? process_count : 50;

    // Выводим процессы
    for (int i = 0; i < display_count; i++) {
        // Преобразуем числовой приоритет в читаемую строку
        const char* priority_str = "Normal";
        if (processes[i].priority == IDLE_PRIORITY_CLASS) priority_str = "Idle";
        else if (processes[i].priority == BELOW_NORMAL_PRIORITY_CLASS) priority_str = "Below";
        else if (processes[i].priority == NORMAL_PRIORITY_CLASS) priority_str = "Normal";
        else if (processes[i].priority == ABOVE_NORMAL_PRIORITY_CLASS) priority_str = "Above";
        else if (processes[i].priority == HIGH_PRIORITY_CLASS) priority_str = "High";
        else if (processes[i].priority == REALTIME_PRIORITY_CLASS) priority_str = "Real";

        // Цвет для процессов с большим потреблением памяти
        const char* color = COLOR_END;
        if (processes[i].memory > 100000) color = COLOR_RED;       // > 100 MB - критично
        else if (processes[i].memory > 50000) color = COLOR_YELLOW; // > 50 MB - средне

        // Проверяем, есть ли у процесса права администратора
        const char* admin_str = "Нет";
        HANDLE hToken = NULL;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processes[i].pid);
        if (hProcess) {
            if (OpenProcessToken(hProcess, TOKEN_QUERY, &hToken)) {
                TOKEN_ELEVATION elevation;
                DWORD size = sizeof(TOKEN_ELEVATION);
                if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
                    admin_str = elevation.TokenIsElevated ? "Да" : "Нет";
                }
                CloseHandle(hToken);
            }
            CloseHandle(hProcess);
        }

        // Выводим информацию о процессе с цветом
        printf("%s%-8d %-45s %-15lu %-10s %-8s%s\n",
            color,
            processes[i].pid,
            processes[i].name,
            processes[i].memory,
            priority_str,
            admin_str,
            COLOR_END);
    }

    // Выводим статистику
    printf("--------------------------------------------------------------------------------------------------------\n");
    printf("Всего процессов: %d (показано %d)\n", process_count, display_count);
}

/*
 * Функция: system_monitor_mode
 * Назначение: Главный режим мониторинга системы (аналог top/htop)
 *
 * Алгоритм работы:
 * 1. Выводит инструкцию по управлению
 * 2. Бесконечный цикл:
 *    a. Показывает текущую информацию
 *    b. Ожидает нажатия клавиши
 *    c. При нажатии '0' - выход
 *    d. При нажатии любой другой клавиши - обновление информации
 * 3. Обновление происходит только по запросу пользователя
 *
 * Управление:
 * - '0' - выход из монитора
 * - Любая другая клавиша - обновить информацию
 *
 * Преимущества:
 * - Экономит ресурсы (не обновляется постоянно)
 * - Удобное управление (как в htop)
 * - Выход по '0' (без ошибок отладчика)
 * - Интуитивно понятное управление
 */
void system_monitor_mode(void) {
    // Основной цикл мониторинга
    while (1) {
        // Очищаем экран для обновления данных
        clear_screen();

        // Выводим заголовок с инструкцией
        printf("%s%s================================================%s\n",
            COLOR_HEADER, COLOR_BOLD, COLOR_END);
        printf("%s%s          МОНИТОР СИСТЕМЫ          %s\n",
            COLOR_HEADER, COLOR_BOLD, COLOR_END);
        printf("%s%s================================================%s\n",
            COLOR_HEADER, COLOR_BOLD, COLOR_END);
        printf("\n");
        printf("  Нажмите %s0%s для выхода\n", COLOR_YELLOW, COLOR_END);
        printf("  Нажмите %sлюбую другую клавишу%s для обновления\n", COLOR_YELLOW, COLOR_END);
        printf("\n");

        // Показываем загрузку CPU и память
        show_cpu_memory();

        // Показываем список процессов (до 50)
        show_real_processes();

        // Выводим приглашение
        printf("\nНажмите %s0%s для выхода\n", COLOR_YELLOW, COLOR_END);
        printf("Нажмите %sлюбую другую клавишу%s для обновления\n", COLOR_YELLOW, COLOR_END);
        printf("\n%sОжидание нажатия клавиши...%s\n", COLOR_CYAN, COLOR_END);

        // Ожидаем нажатия клавиши (блокирующий вызов)
        // _getch() - ожидает и возвращает код нажатой клавиши
        char ch = _getch();

        // Если нажата '0' - выходим из цикла
        if (ch == '0') {
            printf("\n%sВыход из монитора...%s\n", COLOR_GREEN, COLOR_END);
            break;
        }

        // При нажатии любой другой клавиши цикл продолжается
        // и информация обновляется (очистка экрана в начале цикла)
    }

    // Ожидаем нажатия Enter перед возвратом в главное меню
    printf("Нажмите Enter для продолжения...");
    getchar();
}

#endif /* _WIN32 */