#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "value.h"

#ifndef NATIVE_ERROR_API
#define NATIVE_ERROR_API 1
#endif

#define RUNS 5
#define CALLS 30000000

#if NATIVE_ERROR_API
typedef bool (*NativeCall)(
    int argCount, const Value* args, Value* result, const char** error);

static bool nativeAbs(
    int argCount, const Value* args, Value* result, const char** error) {
  (void)argCount;

  if (!IS_NUMBER(args[0])) {
    *error = "abs needs a number.";
    return false;
  }

  *result = NUMBER_VAL(fabs(AS_NUMBER(args[0])));
  return true;
}
#else
typedef Value (*NativeCall)(int argCount, const Value* args);

static Value nativeAbs(int argCount, const Value* args) {
  (void)argCount;

  if (!IS_NUMBER(args[0])) {
    return NIL_VAL;
  }

  return NUMBER_VAL(fabs(AS_NUMBER(args[0])));
}
#endif

static double expectedSum(int count) {
  int remainder = count % 1024;
  return (count / 1024) * 524800.0 +
      remainder * (remainder + 1.0) / 2.0;
}

static double run(int count) {
  NativeCall volatile native = nativeAbs;
  double checksum = 0;

  for (int i = 0; i < count; i++) {
    Value argument = NUMBER_VAL(-(double)((i & 1023) + 1));

#if NATIVE_ERROR_API
    Value result = NIL_VAL;
    const char* error = NULL;

    if (!native(1, &argument, &result, &error)) {
      fprintf(stderr, "%s\n", error != NULL ? error : "Native failed.");
      exit(1);
    }
#else
    Value result = native(1, &argument);
#endif

    checksum += AS_NUMBER(result);
  }

  return checksum;
}

int main(void) {
  double times[RUNS];
  double expected = expectedSum(CALLS);
  double warmup = run(200000);
  assert(warmup == expectedSum(200000));

  printf("NATIVE_ERROR_API=%d; calls per run: %d\n", NATIVE_ERROR_API, CALLS);

  for (int i = 0; i < RUNS; i++) {
    clock_t start = clock();
    double checksum = run(CALLS);
    clock_t finish = clock();

    assert(start != (clock_t)-1);
    assert(finish != (clock_t)-1);
    assert(checksum == expected);

    times[i] = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("run %d: %.6f seconds; checksum: %.0f\n",
        i + 1, times[i], checksum);
  }

  for (int i = 1; i < RUNS; i++) {
    double value = times[i];
    int j = i - 1;

    while (j >= 0 && times[j] > value) {
      times[j + 1] = times[j];
      j--;
    }

    times[j + 1] = value;
  }

  printf("median: %.6f seconds\n", times[RUNS / 2]);
  return 0;
}
