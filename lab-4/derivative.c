#include "derivative.h"

#include <math.h>

float derivative(float A, float deltaX) {
  // Формула центральной разности: (cos(A+deltaX) - cos(A-deltaX)) / (2*deltaX)
  return (cosf(A + deltaX) - cosf(A - deltaX)) / (2.0f * deltaX);
}
