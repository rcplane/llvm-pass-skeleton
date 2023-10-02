int sum(int *data, int len)
{
  int s = 0;
  for (int i = 0; i < len; ++i) {
    s += data[i];
  }
  return s;
}
