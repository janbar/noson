/*
 *      Copyright (C) 2016 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef SECURESOCKET_H
#define SECURESOCKET_H

#include "socket.h"

#include <string>

namespace NSROOT
{
  class SecureSocket;

  class SSLSessionFactory
  {
  public:
    static SSLSessionFactory& Instance();
    static void Destroy();
    bool IsEnabled() const { return m_enabled; }

    /**
     * Create a new client socket.
     * Ownership of the returned pointer is transferred to the caller.
     * @return new pointer to socket or nullptr on failure
     */
    SecureSocket* NewClientSocket();

  private:
    SSLSessionFactory();
    ~SSLSessionFactory();
    SSLSessionFactory(const SSLSessionFactory&);
    SSLSessionFactory& operator=(const SSLSessionFactory&);

    static SSLSessionFactory* m_instance;
    void* m_client_ctx;       ///< SSL default client context
    bool m_enabled;           ///< SSL feature status
  };

  class SSLServerContext
  {
  public:
    SSLServerContext() : m_server_ctx(nullptr) { }
    ~SSLServerContext();

    /**
     * Initialize the server context with the given identity.
     * The certificate and key must match for the operation to succeed.
     * @param certfile the string path of the certificate file (pem)
     * @param pkeyfile the string path of the private key file (pem)
     * @return true on success, else false
     */
    bool InitContext(const std::string& certfile, const std::string& pkeyfile);

    /**
     * Create a new server socket with this context.
     * Ownership of the returned pointer is transferred to the caller.
     * @return new pointer to socket or nullptr on failure
     */
    SecureSocket* NewServerSocket();

  private:
    SSLServerContext(const SSLServerContext& other);
    SSLServerContext& operator=(const SSLServerContext& other);

    void* m_server_ctx;       ///< SSL server context
  };

  class SecureSocket : public TcpSocket
  {
    friend class SSLSessionFactory;
    friend class SSLServerContext;
    friend class SecureServerSocket;
  public:
    virtual ~SecureSocket();

    // Overrides TcpSocket
    bool Connect(const char *server, unsigned port, int rcvbuf);
    bool SendData(const char* buf, size_t size);
    size_t ReceiveData(void* buf, size_t n);
    void Disconnect();

    bool IsCertificateValid(std::string& info);

    const char* GetSSLError();

  private:
    SecureSocket(void* ssl);

    void* m_ssl;      ///< SSL handle
    void* m_cert;     ///< X509 certificate
    bool m_connected; ///< SSL session state
    int m_ssl_error;  ///< SSL error code
    char* m_errmsg;   ///< error message buffer
  };

  class SecureServerSocket
  {
  public:
    /**
     * Await a connection.
     * @param listener the tcp server socket to accept new connection
     * @param socket the secure socket to connect on new request
     * @param timeout in seconds
     * @return AcceptStatus
     */
    static TcpServerSocket::AcceptStatus AcceptConnection(
            TcpServerSocket& listener,
            SecureSocket& socket,
            int timeout);
  private:
    SecureServerSocket() { }
    ~SecureServerSocket() { }
  };
}

#endif /* SECURESOCKET_H */


