#include "node.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
  Функция nodeProcess реализует обработку команд для узла.
  Параметры:
    - id: идентификатор узла
    - isManager: флаг, обозначающий, является ли узел управляющим (не
  используется в данном примере)
    - readFd: файловый дескриптор, из которого читаются команды (конец канала
  для чтения)
*/
void nodeProcess(int id, int isManager, int readFd) {
  KeyVal kvStore[MAX_KEYVAL];  // Локальное хранилище пар ключ-значение
  int kvCount = 0;             // Количество сохранённых пар

  while (1) {
    char buffer[MAX_LINE];
    // Чтение команды из канала; если чтение не удалось – завершаем работу узла
    ssize_t r = read(readFd, buffer, sizeof(buffer) - 1);
    if (r <= 0) break;
    buffer[r] = '\0';

    // Разбираем команду, разделяя строку по пробелам и символам новой строки
    char *cmd = strtok(buffer, " \n");
    if (!cmd) continue;

    if (strcmp(cmd, "exec") == 0) {
      // Обработка команды exec
      char *key = strtok(NULL, " \n");  // Получаем ключ
      char *val = strtok(NULL, " \n");  // Получаем значение (если передано)
      if (!key) continue;
      if (val) {
        // Если значение передано, ищем ключ в локальном хранилище и обновляем
        // его
        int found = 0;
        for (int i = 0; i < kvCount; i++) {
          if (strcmp(kvStore[i].key, key) == 0) {
            kvStore[i].value = atoi(val);
            found = 1;
            break;
          }
        }
        // Если ключ не найден, добавляем новую пару (если есть место)
        if (!found && kvCount < MAX_KEYVAL) {
          strncpy(kvStore[kvCount].key, key, sizeof(kvStore[kvCount].key) - 1);
          kvStore[kvCount].value = atoi(val);
          kvCount++;
        }
        // Выводим сообщение об успешном выполнении операции
        printf("Ok:%d\n", id);
        fflush(stdout);
      } else {
        // Если значение не передано, выполняем операцию чтения
        int found = 0;
        int value = 0;
        for (int i = 0; i < kvCount; i++) {
          if (strcmp(kvStore[i].key, key) == 0) {
            value = kvStore[i].value;
            found = 1;
            break;
          }
        }
        if (found)
          printf("Ok:%d: %d\n", id, value);
        else
          printf("Error:%d '%s' not found\n", id, key);
        fflush(stdout);
      }
    } else if (strcmp(cmd, "ping") == 0) {
      // Команда ping – узел подтверждает свою доступность
      printf("Ok:%d узел %d доступен\n", id, id);
      fflush(stdout);
    } else {
      // Если команда неизвестна – выводим сообщение об ошибке
      printf("Error:%d Unknown command\n", id);
      fflush(stdout);
    }
  }
  // Завершаем процесс, когда канал закрыт или произошла ошибка чтения
  _exit(0);
}
