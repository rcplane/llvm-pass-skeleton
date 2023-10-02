#include "stdio.h"

int sum(int *data, int len)
{
  int s = 0;
  for (int i = 0; i < len; ++i) {
    s += data[i];
  }
  return s;
}

int main() {
  int nums[] = { 1, 2, 3, 4 };
  printf("%d\n", sum(nums, 4));
}
