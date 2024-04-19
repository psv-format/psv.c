#include <stdio.h>
#include <string.h>
#include "config.h"
#include "hello.h"
int main() {
  if(strcmp(HELLO_MESSAGE, "Hello, world") != 0) {
    printf("Unit test failed: Message does not match expected output.\n");
    return 1;
  }
  printf("All Unit Test Passed\n");
  return 0;
}
