#include <stdio.h>
#include <stdlib.h>

#include "derivative.h"
#include "picalc.h"

int main(int argc, char* argv[]) {
  // Проверяем, переданы ли аргументы командной строки.
  // Если аргументов меньше двух, то программа не знает, какую операцию
  // выполнять.
  if (argc < 2) {
    printf("Использование: %s <F1 или F2> [аргументы]\n", argv[0]);
    return 1;
  }

  // Производная функции cos(x).
  if (argv[1][0] == 'F' && argv[1][1] == '1') {
    // Для вычисления производной необходимы два дополнительных параметра: точка
    // A и приращение deltaX.
    if (argc < 4) {
      printf("Пример: %s F1 <A> <deltaX>\n", argv[0]);
      return 1;
    }
    // Преобразуем строковые аргументы в тип float:
    float A = atof(argv[2]);
    float dX = atof(argv[3]);

    float result = derivative(A, dX);
    printf("Производная cos(x) в точке %.3f с приращением %.3f = %.6f\n", A, dX,
           result);
  }

  // Приближение числа π.
  else if (argv[1][0] == 'F' && argv[1][1] == '2') {
    // Для вычисления числа π необходим один параметр: число итераций K для ряда
    // Лейбница.
    if (argc < 3) {
      printf("Пример: %s F2 <K>\n", argv[0]);
      return 1;
    }

    int K = atoi(argv[2]);

    double pi_approx = calculate_pi_leibniz(K);
    printf("Приближение числа π по Лейбницу (K=%d) = %.15f\n", K, pi_approx);
  }
  // Если первый аргумент не соответствует ни "F1", ни "F2", выводим сообщение
  // об ошибке.
  else {
    printf("Неизвестная команда: %s\n", argv[1]);
  }

  return 0;
}
