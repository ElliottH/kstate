#include <stdio.h>

#include "kstate.h"

int main (void)
{

  kstate_state_p state = kstate_new_state();
  printf("1. state: %p\n", state);

  kstate_free_state(&state);
  printf("2. state: %p\n", state);

  return 0;
}
