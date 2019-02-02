#ifndef REQUESTBROKER_H
#define REQUESTBROKER_H

#include "local_config.h"

#include <cstddef>

namespace NSROOT
{

  class RequestBroker
  {
  public:
    virtual ~RequestBroker();
    virtual void HandleRequest(void* handle, const char* url) = 0;

  protected:
    bool Reply(void* handle, const char* data, size_t size);
  };

}

#endif // REQUESTBROKER_H
