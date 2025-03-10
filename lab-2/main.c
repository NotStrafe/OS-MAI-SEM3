#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define N 16
#define MAX_DEPTH 2

int arr[N];

// Структура для передачи параметров в поток
typedef struct {
  int start;
  int count;
  int direction;
  int depth;
} ThreadData;

// Функция для сравнения двух элементов массива и их обмена в зависимости от
// направления
void compare_and_swap(int i, int j, int direction) {
  if (direction == 1) {
    if (arr[i] > arr[j]) {
      int temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  } else {
    if (arr[i] < arr[j]) {
      int temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
    }
  }
}

// Рекурсивное битоническое слияние
// Объединяет два отсортированных подмассива в один отсортированный массив
void bitonic_merge(int start, int count, int direction) {
  if (count > 1) {
    int mid = count / 2;

    for (int i = start; i < start + mid; i++) {
      compare_and_swap(i, i + mid, direction);
    }

    bitonic_merge(start, mid, direction);
    bitonic_merge(start + mid, mid, direction);
  }
}

// Последовательная битоническая сортировка без использования новых потоков
void bitonic_sort_sequential(int start, int count, int direction, int depth) {
  if (count > 1) {
    int mid = count / 2;
    bitonic_sort_sequential(start, mid, 1, depth);
    bitonic_sort_sequential(start + mid, mid, 0, depth);
    bitonic_merge(start, count, direction);
  }
}

// Функция, которую выполняет каждый поток
// Принимает указатель на структуру с параметрами сортировки
void *thread_function(void *arg) {
  // Копируем параметры из переданной структуры в локальную переменную,
  // чтобы быть уверенными, что они останутся валидными до завершения работы
  // функции.
  ThreadData data = *(ThreadData *)arg;

  if (data.count > 1) {
    int mid = data.count / 2;
    if (data.depth < MAX_DEPTH) {
      pthread_t thread_left, thread_right;
      // Создаём локальные структуры для параметров левого и правого подмассивов
      ThreadData left_data = {data.start, mid, 1, data.depth + 1};
      ThreadData right_data = {data.start + mid, mid, 0, data.depth + 1};

      // Создаём поток для сортировки левой половины
      pthread_create(&thread_left, NULL, thread_function, (void *)&left_data);
      // Создаём поток для сортировки правой половины
      pthread_create(&thread_right, NULL, thread_function, (void *)&right_data);
      // Ожидаем завершения обоих потоков
      pthread_join(thread_left, NULL);
      pthread_join(thread_right, NULL);
    } else {
      // Если достигли максимальной глубины, выполняем сортировку
      // последовательно
      bitonic_sort_sequential(data.start, mid, 1, data.depth);
      bitonic_sort_sequential(data.start + mid, mid, 0, data.depth);
    }
    // После сортировки двух половин объединяем их в единый отсортированный
    // массив
    bitonic_merge(data.start, data.count, data.direction);
  }
  return NULL;
}

int main() {
  srand(time(NULL));

  for (int i = 0; i < 15; i++) {
    arr[i] = 1;
  }
  arr[16] = 2;

  printf("Исходный массив:\n");
  for (int i = 0; i < N; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  // Создаём структуру параметров для сортировки всего массива
  ThreadData main_data = {0, N, 1, 0};

  // Создаем поток для выполнения сортировки
  pthread_t main_thread;
  pthread_create(&main_thread, NULL, thread_function, (void *)&main_data);
  // Ожидаем завершения потока сортировки
  pthread_join(main_thread, NULL);

  gettimeofday(&end_time, NULL);
  double elapsed = (end_time.tv_sec - start_time.tv_sec) +
                   (end_time.tv_usec - start_time.tv_usec) / 1000000.0;

  printf("Отсортированный массив:\n");
  for (int i = 0; i < N; i++) {
    printf("%d ", arr[i]);
  }
  printf("\n");

  printf("Затраченное время: %.6f сек.\n", elapsed);

  return 0;
}
