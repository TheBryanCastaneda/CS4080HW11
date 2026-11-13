#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../c/compiler.h"
#include "../c/object.h"
#include "../c/vm.h"

#ifndef VM_LOCAL_IP
#define VM_LOCAL_IP 1
#endif

#define RUNS 5

static void checkResult(double expected) {
  Value result = NIL_VAL;
  ObjString* name = copyString("result", 6);

  assert(tableGet(&vm.globals, OBJ_VAL(name), &result));
  assert(IS_NUMBER(result));
  assert(AS_NUMBER(result) == expected);
  assert(vm.stackTop == vm.stack);
  assert(vm.frameCount == 0);
}

static void benchmark(
    const char* name, const char* source, double expected) {
  initVM();

  ObjFunction* function = compile(source);
  assert(function != NULL);

  assert(interpretFunction(function) == INTERPRET_OK);
  checkResult(expected);

  double times[RUNS];

  for (int i = 0; i < RUNS; i++) {
    clock_t start = clock();
    InterpretResult status = interpretFunction(function);
    clock_t finish = clock();

    assert(start != (clock_t)-1);
    assert(finish != (clock_t)-1);
    assert(status == INTERPRET_OK);
    checkResult(expected);

    times[i] = (double)(finish - start) / CLOCKS_PER_SEC;
    printf("%s run %d: %.6f seconds\n", name, i + 1, times[i]);
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

  printf(
      "%s median: %.6f seconds; result: %.0f\n\n",
      name, times[RUNS / 2], expected);

  freeVM();
}

int main(void) {
  const char* arithmetic =
      "fun arithmetic(n) {"
      "  var total = 0;"
      "  for (var i = 0; i < n; i = i + 1) {"
      "    total = total + (i * 3 + 7);"
      "  }"
      "  return total;"
      "}"
      "var result = arithmetic(2000000);";

  const char* calls =
      "fun step(x) { return x * 3 + 7; }"
      "fun calls(n) {"
      "  var total = 0;"
      "  for (var i = 0; i < n; i = i + 1) {"
      "    total = total + step(i);"
      "  }"
      "  return total;"
      "}"
      "var result = calls(2000000);";

  double count = 2000000.0;
  double expected = 3.0 * count * (count - 1.0) / 2.0 + 7.0 * count;

  printf("VM_LOCAL_IP=%d\n\n", VM_LOCAL_IP);
  benchmark("arithmetic", arithmetic, expected);
  benchmark("calls", calls, expected);

  return 0;
}
