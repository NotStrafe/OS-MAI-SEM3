#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LEN 1024

int main() {
  // Объявляем массивы для хранения имён файлов для дочерних процессов
  char file1[256], file2[256];

  printf("Введите имя файла для child1: ");
  fflush(stdout);  // Обеспечиваем вывод приглашения сразу
  if (!fgets(file1, sizeof(file1), stdin)) {
    perror("fgets file1");
    return 1;
  }

  file1[strcspn(file1, "\n")] = '\0';

  printf("Введите имя файла для child2: ");
  fflush(stdout);
  if (!fgets(file2, sizeof(file2), stdin)) {
    perror("fgets file2");
    return 1;
  }
  file2[strcspn(file2, "\n")] = '\0';

  int pipefd1[2], pipefd2[2];
  if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
    perror("pipe");
    return 1;
  }

  // Порождаем первый дочерний процесс
  pid_t pid1 = fork();
  if (pid1 < 0) {
    perror("fork child1");
    return 1;
  } else if (pid1 == 0) {
    close(pipefd1[1]);
    close(pipefd2[0]);
    close(pipefd2[1]);

    // Готовим строку для передачи дескриптора чтения из pipe1 в дочерний
    // процесс
    char fdStr[16];
    sprintf(fdStr, "%d", pipefd1[0]);

    // Замещаем текущий процесс программой child1
    // Передаем аргументы: имя программы, дескриптор pipe для чтения и имя файла
    // для записи
    execl("./child1", "child1", fdStr, file1, NULL);
    // Если execl вернул управление, значит произошла ошибка
    perror("execl child1");
    exit(1);
  }

  // Порождаем второй дочерний процесс
  pid_t pid2 = fork();
  if (pid2 < 0) {
    perror("fork child2");
    return 1;
  } else if (pid2 == 0) {
    close(pipefd2[1]);
    close(pipefd1[0]);
    close(pipefd1[1]);

    char fdStr[16];
    sprintf(fdStr, "%d", pipefd2[0]);

    execl("./child2", "child2", fdStr, file2, NULL);
    perror("execl child2");
    exit(1);
  }

  close(pipefd1[0]);
  close(pipefd2[0]);

  srand((unsigned int)time(NULL));

  char buffer[MAX_LINE_LEN];
  printf("Введите строки (Ctrl+D или пустая строка для выхода):\n");

  while (1) {
    printf("> ");
    fflush(stdout);

    if (!fgets(buffer, MAX_LINE_LEN, stdin)) {
      break;
    }
    if (strcmp(buffer, "\n") == 0) {
      break;
    }

    int r = rand() % 100;
    if (r < 80) {
      write(pipefd1[1], buffer, strlen(buffer));
    } else {
      write(pipefd2[1], buffer, strlen(buffer));
    }
  }

  close(pipefd1[1]);
  close(pipefd2[1]);

  wait(NULL);

  printf("Родитель завершил работу.\n");
  return 0;
}
