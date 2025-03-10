#include "picalc.h"

double calculate_pi_leibniz(int K) {
  // Ряд Лейбница: pi = 4 * (1 - 1/3 + 1/5 - 1/7 + ...)
  double sum = 0.0;
  for (int n = 0; n < K; n++) {
    double term = (n % 2 == 0) ? 1.0 : -1.0;
    sum += term / (2.0 * n + 1.0);
  }
  return 4.0 * sum;
}
