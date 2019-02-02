#include "requestbroker.h"

#include "private/socket.h"

using namespace NSROOT;

RequestBroker::~RequestBroker() { }

bool RequestBroker::Reply(void *handle, const char *data, size_t size)
{
  TcpSocket* socket = static_cast<TcpSocket*>(handle);
  if (socket)
    return socket->SendData(data, size);
  return false;
}
