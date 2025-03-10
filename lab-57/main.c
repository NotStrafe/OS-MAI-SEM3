// Определяем POSIX-расширения для корректной работы функций сигналов
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "node.h"

// Максимальное количество узлов, которые можно создать
#define MAX_NODES 100

// Структура для хранения информации об узле (процессе)
typedef struct {
  int id;         // Уникальный идентификатор узла
  int parentId;   // Идентификатор родительского узла (-1 для управляющего)
  int isManager;  // Флаг: 1 — управляющий, 0 — вычислительный
  int alive;      // Статус узла: 1 — активен, 0 — завершён
  pid_t pid;      // PID процесса узла
  int pipefd[2];  // Канал связи: [0] для чтения, [1] для записи
} NodeInfo;

// Массив для хранения информации об узлах и счётчик созданных узлов
static NodeInfo nodes[MAX_NODES];
static int nodeCount = 0;

// Функция поиска индекса узла по его id
int findNodeIndexById(int id) {
  for (int i = 0; i < nodeCount; i++) {
    if (nodes[i].id == id && nodes[i].alive) return i;
  }
  return -1;
}

void recreateNode(int idx);

// Функция создания узла (процесса)
void createNode(int id, int parentId, int isManager) {
  // Если узел с таким id уже существует, выводим ошибку
  if (findNodeIndexById(id) != -1) {
    printf("Error: Node %d already exists\n", id);
    return;
  }
  if (nodeCount >= MAX_NODES) {
    printf("Error: Too many nodes\n");
    return;
  }

  // Создаем канал для связи между родительским и дочерним процессом
  int fds[2];
  if (pipe(fds) == -1) {
    perror("pipe");
    return;
  }

  // Создаем дочерний процесс
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }
  if (pid == 0) {
    // В дочернем процессе закрываем дескриптор записи и запускаем nodeProcess
    close(fds[1]);
    nodeProcess(id, isManager, fds[0]);
    _exit(0);
  } else {
    // В родительском процессе закрываем дескриптор чтения
    close(fds[0]);
    // Сохраняем информацию о новом узле
    nodes[nodeCount].id = id;
    nodes[nodeCount].parentId = parentId;
    nodes[nodeCount].isManager = isManager;
    nodes[nodeCount].alive = 1;
    nodes[nodeCount].pid = pid;
    nodes[nodeCount].pipefd[0] = -1;      // Родитель не читает из этого конца
    nodes[nodeCount].pipefd[1] = fds[1];  // Используется для отправки команд
    nodeCount++;

    printf("Ok:%d pid=%d\n", id, pid);
  }
}

// Функция отправки команды узлу через канал
void sendExecCommand(int nodeIdx, const char *cmd) {
  int len = strlen(cmd);
  if (write(nodes[nodeIdx].pipefd[1], cmd, len) < 0) {
    perror("write to child");
  }
}

// Обработчик сигнала SIGCHLD – вызывается при завершении дочерних процессов
void sigchld_handler(int signo) {
  (void)signo;
  pid_t p;
  int status;
  // Обрабатываем все завершившиеся дочерние процессы (без блокировки)
  while ((p = waitpid(-1, &status, WNOHANG)) > 0) {
    for (int i = 0; i < nodeCount; i++) {
      if (nodes[i].pid == p && nodes[i].alive) {
        int diedId = nodes[i].id;
        nodes[i].alive = 0;
        close(nodes[i].pipefd[1]);
        // Восстанавливаем узел, вызвав функцию recreateNode
        recreateNode(i);
        printf("Node %d (pid=%d) was killed, recreated\n", diedId, p);
        break;
      }
    }
  }
}

// Функция восстановления узла – создает новый процесс с теми же параметрами
void recreateNode(int idx) {
  int id = nodes[idx].id;
  int parentId = nodes[idx].parentId;
  int isMgr = nodes[idx].isManager;
  nodes[idx].alive = 0;
  createNode(id, parentId, isMgr);
}

int main() {
  // Настраиваем обработчик сигнала SIGCHLD для отслеживания завершения дочерних
  // процессов
  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD, &sa, NULL);

  char line[MAX_LINE];
  printf("Введите команды (create / exec / ping / remove и т.д.)\n");
  // Основной цикл чтения и обработки команд с клавиатуры
  while (1) {
    if (!fgets(line, sizeof(line), stdin)) break;
    line[strcspn(line, "\n")] = '\0';
    char *cmd = strtok(line, " ");
    if (!cmd) continue;

    if (strcmp(cmd, "create") == 0) {
      // Команда create: create <id> <parent>
      char *idStr = strtok(NULL, " ");
      char *parStr = strtok(NULL, " ");
      if (!idStr || !parStr) {
        printf("Error: недостаточно аргументов\n");
        continue;
      }
      int id = atoi(idStr);
      int parent = atoi(parStr);
      int isMgr = (parent == -1) ? 1 : 0;
      createNode(id, parent, isMgr);
    } else if (strcmp(cmd, "exec") == 0) {
      // Команда exec: exec <id> <key> [value]
      char *idStr = strtok(NULL, " ");
      if (!idStr) {
        printf("Error: no id\n");
        continue;
      }
      int id = atoi(idStr);
      int idx = findNodeIndexById(id);
      if (idx < 0) {
        printf("Error: Node %d not found\n", id);
        continue;
      }
      char *key = strtok(NULL, " ");
      if (!key) {
        printf("Error:%d no key\n", id);
        continue;
      }
      char *val = strtok(NULL, " ");
      char buf[128];
      if (val)
        snprintf(buf, sizeof(buf), "exec %s %s\n", key, val);
      else
        snprintf(buf, sizeof(buf), "exec %s\n", key);
      sendExecCommand(idx, buf);
    } else if (strcmp(cmd, "ping") == 0) {
      // Команда ping: ping <id>
      char *idStr = strtok(NULL, " ");
      if (!idStr) {
        printf("Error: no id\n");
        continue;
      }
      int id = atoi(idStr);
      int idx = findNodeIndexById(id);
      if (idx < 0) {
        printf("Error: / узел %d недоступен\n", id);
      } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "ping %d\n", id);
        sendExecCommand(idx, buf);
      }
    } else {
      // Неизвестная команда
      printf("Error: Unknown command '%s'\n", cmd);
    }
  }
  return 0;
}
