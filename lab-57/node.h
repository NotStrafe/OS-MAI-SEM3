#ifndef NODE_H
#define NODE_H

// Максимальное количество пар "ключ-значение" в узле
#define MAX_KEYVAL 100
// Максимальная длина строки для обмена командами
#define MAX_LINE 256

// Структура для хранения пары "ключ-значение" в узле
typedef struct {
  char key[64];
  int value;
} KeyVal;

// Объявление функции, реализующей логику работы узла (процесса)
void nodeProcess(int id, int isManager, int readFd);

#endif  // NODE_H
