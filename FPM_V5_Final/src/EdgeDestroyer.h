#ifndef EDGEDESTROYER_H
#define EDGEDESTROYER_H

#include "argraph.h"

class EdgeDestroyer: public AttrDestroyer {
  public:
    virtual void destroy(void *p);
};

#endif

