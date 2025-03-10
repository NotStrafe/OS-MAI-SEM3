#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

// Определяем тип указателя на функцию, которая принимает два аргумента типа
// float и возвращает float. Это соответствует функции вычисления производной,
// реализованной в динамической библиотеке libderivative.so.
typedef float (*derivative_func)(float, float);

typedef double (*picalc_func)(int);

int main(int argc, char* argv[]) {
  // Если аргументов меньше 2 (то есть нет ни команды, ни дополнительных
  // параметров), выводим инструкцию по использованию.
  if (argc < 2) {
    printf("Использование: %s <F1 или F2> [аргументы]\n", argv[0]);
    return 1;
  }

  if (argv[1][0] == 'F' && argv[1][1] == '1') {
    // Для команды F1 ожидается, что будет передано как минимум 3 аргумента: имя
    // программы, команда F1, и два параметра: A и deltaX.
    if (argc < 4) {
      printf("Пример: %s F1 <A> <deltaX>\n", argv[0]);
      return 1;
    }

    // Открываем динамическую библиотеку libderivative.so.
    void* handle = dlopen("./libderivative.so", RTLD_LAZY);
    if (!handle) {
      printf("Не удалось загрузить libderivative.so: %s\n", dlerror());
      return 1;
    }

    // Получаем адрес функции derivative из загруженной библиотеки с помощью
    // dlsym. Приводим возвращаемое значение к типу derivative_func.
    derivative_func derivative = (derivative_func)dlsym(handle, "derivative");
    if (!derivative) {
      printf("Не найдена функция derivative: %s\n", dlerror());
      dlclose(handle);
      return 1;
    }

    // Преобразуем строковые аргументы в числа:
    float A = atof(argv[2]);
    float dX = atof(argv[3]);

    float result = derivative(A, dX);

    printf("Производная cos(x) в точке %.3f с приращением %.3f = %.6f\n", A, dX,
           result);

    dlclose(handle);
  }

  else if (argv[1][0] == 'F' && argv[1][1] == '2') {
    // Для команды F2 ожидается, что будет передан как минимум 2 аргумента: имя
    // программы и параметр K.
    if (argc < 3) {
      printf("Пример: %s F2 <K>\n", argv[0]);
      return 1;
    }

    // Открываем динамическую библиотеку libpicalc.so.
    void* handle = dlopen("./libpicalc.so", RTLD_LAZY);
    if (!handle) {
      printf("Не удалось загрузить libpicalc.so: %s\n", dlerror());
      return 1;
    }

    // Получаем адрес функции calculate_pi_leibniz из загруженной библиотеки.
    picalc_func calculate_pi_leibniz =
        (picalc_func)dlsym(handle, "calculate_pi_leibniz");
    if (!calculate_pi_leibniz) {
      printf("Не найдена функция calculate_pi_leibniz: %s\n", dlerror());
      dlclose(handle);
      return 1;
    }

    int K = atoi(argv[2]);

    double pi_approx = calculate_pi_leibniz(K);

    printf("Приближение числа π по Лейбницу (K=%d) = %.15f\n", K, pi_approx);

    dlclose(handle);
  }

  else {
    printf("Неизвестная команда: %s\n", argv[1]);
  }

  return 0;
}
