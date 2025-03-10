#define _POSIX_C_SOURCE 199309L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_JOBS 100
#define MAX_DEPS 20
#define NAME_LEN 64

// Статусы выполнения джобы
typedef enum { JOB_NOT_STARTED, JOB_RUNNING, JOB_DONE, JOB_FAILED } JobStatus;

// Структура, описывающая джобу
typedef struct {
  char name[NAME_LEN];                    // Имя джобы, например "job-1"
  char dependencies[MAX_DEPS][NAME_LEN];  // Имена джоб, от которых зависит
  int depCount;                           // Число зависимостей
  int depRemaining;  // Сколько зависимостей ещё не выполнено
  JobStatus status;  // Текущий статус джобы
  pthread_t thread;  // Идентификатор потока

  // Для измерения времени выполнения
  struct timespec startTime;
  struct timespec endTime;
  double duration;  // В секундах
} Job;

// Глобальные переменные
static Job g_jobs[MAX_JOBS];
static int g_jobCount = 0;  // Количество джоб

// Флаг аварийного прерывания
static volatile int g_abortFlag = 0;

// Мьютекс и условная переменная для синхронизации
static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cond = PTHREAD_COND_INITIALIZER;

// Вспомогательные функции

// Поиск индекса джобы в массиве g_jobs по имени
int findJobIndex(const char* name) {
  for (int i = 0; i < g_jobCount; i++) {
    if (strcmp(g_jobs[i].name, name) == 0) return i;
  }
  return -1;
}

// Функция для парсинга INI-файла.а
void parseIniFile(const char* filename) {
  FILE* f = fopen(filename, "r");
  if (!f) {
    printf("Не удалось открыть файл: %s\n", filename);
    exit(1);
  }

  char line[256];
  char currentJobName[NAME_LEN] = {0};

  while (fgets(line, sizeof(line), f)) {
    // Убираем символ перевода строки
    char* nl = strchr(line, '\n');
    if (nl) *nl = '\0';

    // Пропускаем пустые строки
    if (strlen(line) == 0) continue;

    // Если строка начинается с '[', значит начинается новая секция с именем
    // джобы
    if (line[0] == '[') {
      char* rb = strchr(line, ']');
      if (!rb) continue;  // Некорректный формат
      memset(currentJobName, 0, NAME_LEN);
      strncpy(currentJobName, line + 1, rb - (line + 1));

      // Добавляем новую джобу в массив
      strcpy(g_jobs[g_jobCount].name, currentJobName);
      g_jobs[g_jobCount].depCount = 0;
      g_jobs[g_jobCount].depRemaining = 0;
      g_jobs[g_jobCount].status = JOB_NOT_STARTED;
      g_jobs[g_jobCount].duration = 0.0;
      g_jobCount++;
    }
    // Ищем строку "depends = ..." в секции
    else {
      char* dependsPtr = strstr(line, "depends");
      if (dependsPtr) {
        char* eq = strchr(line, '=');
        if (eq) {
          char depsLine[256];
          strcpy(depsLine, eq + 1);

          // Удаляем пробелы в начале строки зависимостей
          char* start = depsLine;
          while (*start == ' ' || *start == '\t') start++;

          int idx = findJobIndex(currentJobName);
          if (idx == -1) continue;

          if (strlen(start) == 0) {
            // Зависимостей нет
            g_jobs[idx].depCount = 0;
            continue;
          }

          // Разбиваем строку по запятым
          char* token = strtok(start, ",");
          while (token && g_jobs[idx].depCount < MAX_DEPS) {
            while (*token == ' ' || *token == '\t') token++;
            strncpy(g_jobs[idx].dependencies[g_jobs[idx].depCount], token,
                    NAME_LEN);
            g_jobs[idx].depCount++;
            token = strtok(NULL, ",");
          }
        }
      }
    }
  }
  fclose(f);
}

// Проверка наличия циклов в DAG с помощью алгоритма Канна
int checkForCycles() {
  // Изначально depRemaining равен количеству зависимостей (depCount)
  for (int i = 0; i < g_jobCount; i++) {
    g_jobs[i].depRemaining = g_jobs[i].depCount;
  }

  int removedCount = 0;
  int removed[MAX_JOBS];
  memset(removed, 0, sizeof(removed));

  while (1) {
    int foundZero = -1;
    for (int i = 0; i < g_jobCount; i++) {
      if (!removed[i] && g_jobs[i].depRemaining == 0) {
        foundZero = i;
        break;
      }
    }
    if (foundZero == -1) break;  // Нет свободной джобы для удаления

    removed[foundZero] = 1;
    removedCount++;

    // Уменьшаем значение depRemaining для джоб, зависящих от найденной
    for (int j = 0; j < g_jobCount; j++) {
      if (!removed[j]) {
        for (int d = 0; d < g_jobs[j].depCount; d++) {
          if (strcmp(g_jobs[j].dependencies[d], g_jobs[foundZero].name) == 0)
            g_jobs[j].depRemaining--;
        }
      }
    }
  }

  // Если удалили не все, значит есть цикл
  return (removedCount < g_jobCount) ? 1 : 0;
}

// Вычисление разницы времени (end - start) в секундах
double timeDiffSec(struct timespec start, struct timespec end) {
  double sec = (double)(end.tv_sec - start.tv_sec);
  double nsec = (double)(end.tv_nsec - start.tv_nsec) / 1e9;
  return sec + nsec;
}

// Функция потока для выполнения джобы

void* jobRunner(void* arg) {
  int jobIndex = *(int*)arg;
  free(arg);

  // Начинаем выполнение: фиксируем статус и время старта
  pthread_mutex_lock(&g_mutex);
  if (g_abortFlag) {
    pthread_mutex_unlock(&g_mutex);
    pthread_exit(NULL);
  }
  g_jobs[jobIndex].status = JOB_RUNNING;
  timespec_get(&g_jobs[jobIndex].startTime, TIME_UTC);
  pthread_mutex_unlock(&g_mutex);

  // Имитация работы джобы: случайное время от 1 до 3 секунд
  sleep(rand() % 3 + 1);

  // С вероятностью 1 к 5 джоба завершается с ошибкой
  int failChance = rand() % 5;
  if (failChance == 0) {
    pthread_mutex_lock(&g_mutex);
    g_jobs[jobIndex].status = JOB_FAILED;
    g_abortFlag = 1;
    timespec_get(&g_jobs[jobIndex].endTime, TIME_UTC);
    g_jobs[jobIndex].duration =
        timeDiffSec(g_jobs[jobIndex].startTime, g_jobs[jobIndex].endTime);

    // Отмена остальных запущенных потоков
    for (int i = 0; i < g_jobCount; i++) {
      if (i != jobIndex && g_jobs[i].status == JOB_RUNNING)
        pthread_cancel(g_jobs[i].thread);
    }
    pthread_mutex_unlock(&g_mutex);
    pthread_cond_broadcast(&g_cond);
    pthread_exit(NULL);
  }

  // Если джоба выполнена успешно
  pthread_mutex_lock(&g_mutex);
  g_jobs[jobIndex].status = JOB_DONE;
  timespec_get(&g_jobs[jobIndex].endTime, TIME_UTC);
  g_jobs[jobIndex].duration =
      timeDiffSec(g_jobs[jobIndex].startTime, g_jobs[jobIndex].endTime);

  // Уменьшаем depRemaining для джоб, зависящих от текущей
  for (int j = 0; j < g_jobCount; j++) {
    if (g_jobs[j].status == JOB_NOT_STARTED) {
      for (int d = 0; d < g_jobs[j].depCount; d++) {
        if (strcmp(g_jobs[j].dependencies[d], g_jobs[jobIndex].name) == 0)
          g_jobs[j].depRemaining--;
      }
    }
  }

  pthread_cond_broadcast(&g_cond);
  pthread_mutex_unlock(&g_mutex);
  return NULL;
}

// Главная функция

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Использование: %s <ini-файл>\n", argv[0]);
    return 1;
  }

  srand((unsigned int)time(NULL));

  // 1. Парсинг INI-файла
  parseIniFile(argv[1]);
  if (g_jobCount == 0) {
    printf("Не найдено ни одной джобы.\n");
    return 1;
  }

  // 2. Проверка DAG на циклы
  if (checkForCycles()) {
    printf("Ошибка: в DAG обнаружен цикл!\n");
    return 1;
  }

  // Восстанавливаем depRemaining после проверки циклов
  for (int i = 0; i < g_jobCount; i++) {
    g_jobs[i].depRemaining = g_jobs[i].depCount;
  }

  // Измеряем общее время работы планировщика
  struct timespec globalStart, globalEnd;
  timespec_get(&globalStart, TIME_UTC);

  // 3. Запуск джоб с учётом зависимостей
  int completedJobs = 0;
  while (1) {
    pthread_mutex_lock(&g_mutex);

    // Если уже произошёл сбой, прерываем выполнение
    if (g_abortFlag) {
      pthread_mutex_unlock(&g_mutex);
      break;
    }

    // Ищем джобы, готовые к запуску (depRemaining == 0 и статус NOT_STARTED)
    int startedSomething = 0;
    for (int i = 0; i < g_jobCount; i++) {
      if (g_jobs[i].status == JOB_NOT_STARTED && g_jobs[i].depRemaining == 0) {
        int* idxPtr = malloc(sizeof(int));
        *idxPtr = i;
        pthread_create(&g_jobs[i].thread, NULL, jobRunner, idxPtr);
        startedSomething = 1;
      }
    }

    // Если не найдено ни одной новой джобы, проверяем, завершены ли все
    if (!startedSomething) {
      completedJobs = 0;
      for (int i = 0; i < g_jobCount; i++) {
        if (g_jobs[i].status == JOB_DONE || g_jobs[i].status == JOB_FAILED)
          completedJobs++;
      }
      if (completedJobs == g_jobCount) {
        pthread_mutex_unlock(&g_mutex);
        break;
      }
    }

    // Ожидаем сигнала о завершении хотя бы одной джобы
    pthread_cond_wait(&g_cond, &g_mutex);
    pthread_mutex_unlock(&g_mutex);
  }

  // 4. Дожидаемся завершения всех потоков
  for (int i = 0; i < g_jobCount; i++) {
    if (g_jobs[i].status == JOB_RUNNING) pthread_cancel(g_jobs[i].thread);
  }
  for (int i = 0; i < g_jobCount; i++) {
    if (g_jobs[i].thread) pthread_join(g_jobs[i].thread, NULL);
  }

  timespec_get(&globalEnd, TIME_UTC);
  double totalTime = timeDiffSec(globalStart, globalEnd);

  // Подсчитываем количество неудачных джоб
  int failedCount = 0;
  for (int i = 0; i < g_jobCount; i++) {
    if (g_jobs[i].status == JOB_FAILED) failedCount++;
  }

  // Вывод результатов
  if (failedCount > 0)
    printf("DAG завершён с ошибкой. Упавших джоб: %d\n", failedCount);
  else
    printf("Все джобы успешно завершились!\n");

  printf("\nОбщее время выполнения: %.2f сек.\n", totalTime);
  printf("Статистика по джобам:\n");
  for (int i = 0; i < g_jobCount; i++) {
    printf("  %s: статус=%s, время=%.2f сек\n", g_jobs[i].name,
           (g_jobs[i].status == JOB_DONE)      ? "DONE"
           : (g_jobs[i].status == JOB_FAILED)  ? "FAILED"
           : (g_jobs[i].status == JOB_RUNNING) ? "RUNNING"
                                               : "NOT_STARTED",
           g_jobs[i].duration);
  }

  return 0;
}
