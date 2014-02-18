#include <iostream>
#include "EdgeDestroyer.h"

void EdgeDestroyer::destroy(void *p){
  std::cout << "p: " << *(int*)p << std::endl;

  delete (int*)p;
}

