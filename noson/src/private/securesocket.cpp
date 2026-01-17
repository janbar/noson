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

#include "securesocket.h"
#include "debug.h"

#include <errno.h>

#ifdef __WINDOWS__
#include <WinSock2.h>
#define LASTERROR WSAGetLastError()
#define ERRNO_INTR WSAEINTR
#else
#define LASTERROR errno
#define ERRNO_INTR EINTR
#endif /* __WINDOWS__ */

using namespace NSROOT;

SSLSessionFactory* SSLSessionFactory::m_instance = 0;

SSLSessionFactory& SSLSessionFactory::Instance()
{
  if (!m_instance)
    m_instance = new SSLSessionFactory();
  return *m_instance;
}

void SSLSessionFactory::Destroy()
{
  if (m_instance)
    delete m_instance;
  m_instance = nullptr;
}

#if HAVE_OPENSSL

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

/* Cipher suites, https://www.openssl.org/docs/apps/ciphers.html */
const char* const PREFERRED_CIPHERS = "HIGH:!aNULL:!PSK:!SRP:!MD5:!RC4:!CAMELLIA:!DSS";

SSLSessionFactory::SSLSessionFactory()
: m_client_ctx(nullptr)
, m_enabled(false)
{
  if (SSL_library_init() < 0)
    DBG(DBG_ERROR, "%s: Could not initialize the SSL library\n", __FUNCTION__);
  else
  {
    SSL_load_error_strings();
    /* SSL_load_error_strings loads both libssl and libcrypto strings */
    /* ERR_load_crypto_strings(); */

    /* Setup the default client context */
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
#else
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
#endif
    if (!ctx)
      DBG(DBG_ERROR, "%s: Could not create the SSL client context\n", __FUNCTION__);
    else
    {
      SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

      /* Remove the most egregious. Because SSLv2 and SSLv3 have been removed,
       * a TLSv1.0 handshake is used. The client accepts TLSv1.0 and above.
       * An added benefit of TLS 1.0 and above are TLS extensions like Server
       * Name Indicatior (SNI).
       */
      const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
      (void)SSL_CTX_set_options(ctx, flags);

      /* Each cipher suite takes 2 bytes in the ClientHello, so advertising every
       * cipher suite available at the client is going to cause a big ClientHello
       * (or bigger then needed to get the job done).
       * When using SSL_CTX_set_cipher_list or SSL_set_cipher_list with the string
       * below you'll cut the number of cipher suites down to about 45.
       */
      if (SSL_CTX_set_cipher_list(ctx, PREFERRED_CIPHERS) != 1)
        DBG(DBG_ERROR, "%s: Set cipher list failed\n", __FUNCTION__);

      /* The SSL trace callback is only used for verbose logging */
      /* SSL_CTX_set_msg_callback(ctx, ssl_trace); */

      m_client_ctx = ctx;
      m_enabled = true;
      DBG(DBG_DEBUG, "%s: SSL has been initialized\n", __FUNCTION__);
    }
  }
}

SSLSessionFactory::~SSLSessionFactory()
{
  if (m_client_ctx)
    SSL_CTX_free(static_cast<SSL_CTX*>(m_client_ctx));
  ERR_free_strings();
  EVP_cleanup();
  DBG(DBG_INFO, "%s: SSL resources destroyed\n", __FUNCTION__);
}

SecureSocket* SSLSessionFactory::NewClientSocket()
{
  if (!m_client_ctx)
    return nullptr;
  SSL* ssl = SSL_new(static_cast<SSL_CTX*>(m_client_ctx));
  /* SSL_MODE_AUTO_RETRY
   * With this option set, if the server suddenly wants a new handshake,
   * OpenSSL handles it in the background. Without this option, any read
   * or write operation will return an error if the server wants a new
   * handshake, setting the retry flag in the process.
   */
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  return new SecureSocket(ssl);
}

SSLServerContext::~SSLServerContext()
{
  if (m_server_ctx)
  {
    DBG(DBG_DEBUG, "%s: Free SSL server context (%p)\n", __FUNCTION__, m_server_ctx);
    SSL_CTX_free(static_cast<SSL_CTX*>(m_server_ctx));
  }
}

bool SSLServerContext::InitContext(const std::string& certfile, const std::string& pkeyfile)
{
  if (m_server_ctx || !SSLSessionFactory::Instance().IsEnabled())
    return false;

  /* Setup server context */
#if (OPENSSL_VERSION_NUMBER >= 0x10100000L)
  SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
#else
  SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
#endif
  if (!ctx)
  {
    DBG(DBG_ERROR, "%s: Could not create the SSL server context\n", __FUNCTION__);
    return false;
  }

  SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

  /* Remove the most egregious */
  const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
  (void)SSL_CTX_set_options(ctx, flags);

  if (SSL_CTX_set_cipher_list(ctx, "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2") != 1)
    DBG(DBG_ERROR, "%s: Set cipher list failed\n", __FUNCTION__);

  /* The SSL trace callback is only used for verbose logging */
  /* SSL_CTX_set_msg_callback(ctx, ssl_trace); */

  /* Set the certificate to be used */
  if (SSL_CTX_use_certificate_chain_file(ctx, certfile.c_str()) != 1)
  {
    DBG(DBG_ERROR, "%s: Certificate file is invalid\n", __FUNCTION__);
    SSL_CTX_free(ctx);
    return false;
  }
  /* Set the private key to be used */
  if (SSL_CTX_use_PrivateKey_file(ctx, pkeyfile.c_str(), SSL_FILETYPE_PEM) != 1)
  {
    DBG(DBG_ERROR, "%s: Private key file is invalid\n", __FUNCTION__);
    SSL_CTX_free(ctx);
    return false;
  }
  /* Make sure the key and certificate file match */
  if (SSL_CTX_check_private_key(ctx) != 1) {
    DBG(DBG_ERROR, "%s: Private key does not match the certificate public key\n", __FUNCTION__);
    SSL_CTX_free(ctx);
    return false;
  }
  DBG(DBG_INFO, "%s: Server certificate was successfully loaded\n", __FUNCTION__);
  m_server_ctx = ctx;
  return true;
}

SecureSocket* SSLServerContext::NewServerSocket()
{
  if (!m_server_ctx)
    return nullptr;
  SSL* ssl = SSL_new(static_cast<SSL_CTX*>(m_server_ctx));
  /* SSL_MODE_AUTO_RETRY
   * With this option set, if the server suddenly wants a new handshake,
   * OpenSSL handles it in the background. Without this option, any read
   * or write operation will return an error if the server wants a new
   * handshake, setting the retry flag in the process.
   */
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  return new SecureSocket(ssl);
}

SecureSocket::SecureSocket(void* ssl)
: TcpSocket()
, m_ssl(ssl)
, m_cert(nullptr)
, m_connected(false)
, m_ssl_error(0)
, m_errmsg(nullptr)
{
}

SecureSocket::~SecureSocket()
{
  Disconnect();
  SSL_free(static_cast<SSL*>(m_ssl));
  if (m_errmsg)
    delete [] m_errmsg;
}

bool SecureSocket::Connect(const char* server, unsigned port, int rcvbuf)
{
  m_ssl_error = 0;
  if (m_connected)
    Disconnect();

  /* Connect the tcp socket to the server */
  if (!TcpSocket::Connect(server, port, rcvbuf))
    return false;

  /* setup SSL */
  SSL_set_fd(static_cast<SSL*>(m_ssl), m_socket);
  SSL_set_tlsext_host_name(static_cast<SSL*>(m_ssl), server); /* fix SNI */

  /* do SSL handshake */
  for (;;)
  {
    int r = SSL_connect(static_cast<SSL*>(m_ssl));
    if (r > 0)
      break;
    if (r < 0)
    {
      int err = SSL_get_error(static_cast<SSL*>(m_ssl), r);
      if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
      {
        DBG(DBG_DEBUG, "%s: SSL retry (%d)\n", __FUNCTION__, err);
        continue;
      }
    }
    m_ssl_error = ERR_get_error();
    DBG(DBG_ERROR, "%s: SSL connect failed: %s\n", __FUNCTION__, GetSSLError());
    TcpSocket::Disconnect();
    return false;
  }
  DBG(DBG_PROTO, "%s: SSL handshake initialized\n", __FUNCTION__);
  m_connected = true;
  /* check for a valid certificate */
  std::string str("");
  if (!IsCertificateValid(str))
  {
    DBG(DBG_ERROR, "%s: Could not get a valid certificate from the server\n", __FUNCTION__);
    Disconnect();
  }
  DBG(DBG_PROTO, "%s: %s\n", __FUNCTION__, str.c_str());
  return true;
}

size_t SecureSocket::ReceiveData(void* buf, size_t n)
{
  if (m_connected && n > 0)
  {
    m_ssl_error = SSL_ERROR_NONE;
    for (;;)
    {
      if (SSL_pending(static_cast<SSL*>(m_ssl)) == 0)
      {
        int hangcount = 0;
        for (;;)
        {
          int s = TcpSocket::Listen(&m_timeout);
          if (s > 0)
            break;
          else if (s == 0)
          {
            DBG(DBG_WARN, "%s: socket(%p) timed out (%d)\n", __FUNCTION__, &m_socket, hangcount);
            m_errno = ETIMEDOUT;
            if (++hangcount >= m_attempt)
              return 0;
          }
          else if (m_errno != ERRNO_INTR)
            return 0;
        }
      }

      int r = SSL_read(static_cast<SSL*>(m_ssl), buf, (int) n);
      if (r >= 0)
        return (size_t) r;
      int err = SSL_get_error(static_cast<SSL*>(m_ssl), r);
      if (err == SSL_ERROR_WANT_READ)
      {
        DBG(DBG_DEBUG, "%s: SSL retry\n", __FUNCTION__);
        continue;
      }
      if (err == SSL_ERROR_WANT_WRITE)
      {
        DBG(DBG_DEBUG, "%s: SSL wants write\n", __FUNCTION__);
        m_ssl_error = ERR_get_error();
        break;
      }
      m_ssl_error = ERR_get_error();
      DBG(DBG_ERROR, "%s: SSL read failed: %s\n", __FUNCTION__, GetSSLError());
      break;
    }
  }
  return 0;
}

bool SecureSocket::SendData(const char* buf, size_t size)
{
  if (m_connected && size > 0)
  {
    m_ssl_error = SSL_ERROR_NONE;
    for (;;)
    {
      int r = SSL_write(static_cast<SSL*>(m_ssl), buf, (int) size);
      if (r > 0 && size == (size_t) r)
        return true;
      int err = SSL_get_error(static_cast<SSL*>(m_ssl), r);
      if (err == SSL_ERROR_WANT_WRITE)
      {
        DBG(DBG_DEBUG, "%s: SSL retry\n", __FUNCTION__);
        continue;
      }
      if (err == SSL_ERROR_WANT_READ)
      {
        DBG(DBG_DEBUG, "%s: SSL wants read\n", __FUNCTION__);
        m_ssl_error = ERR_get_error();
        break;
      }
      m_ssl_error = ERR_get_error();
      DBG(DBG_ERROR, "%s: SSL write failed: %s\n", __FUNCTION__, GetSSLError());
      break;
    }
  }
  return false;
}

void SecureSocket::Disconnect()
{
  if (m_connected)
  {
    SSL_shutdown(static_cast<SSL*>(m_ssl));
    m_connected = false;
  }
  TcpSocket::Disconnect();
  if (m_cert)
  {
    X509_free(static_cast<X509*>(m_cert));
    m_cert = nullptr;
  }
}

bool SecureSocket::IsCertificateValid(std::string& str)
{
  if (m_cert)
    X509_free(static_cast<X509*>(m_cert));
  m_cert = SSL_get_peer_certificate(static_cast<SSL*>(m_ssl));
  if (m_cert)
  {
    char buf[80];
    // X509_get_subject_name() returns the subject name of certificate x.
    // The returned value is an internal pointer which MUST NOT be freed.
    X509_NAME* name = X509_get_subject_name(static_cast<X509*>(m_cert));
    str.assign(X509_NAME_oneline(name, buf, sizeof(buf) - 1));
    return true;
  }
  return false;
}

#define ERROR_MSG_SIZE  256
const char* SecureSocket::GetSSLError()
{
  // create error message buffer as needed
  if (!m_errmsg)
    m_errmsg = new char[ERROR_MSG_SIZE];
  ERR_error_string_n(m_ssl_error, m_errmsg, ERROR_MSG_SIZE);
  return m_errmsg;
}

TcpServerSocket::AcceptStatus SecureServerSocket::AcceptConnection(
        TcpServerSocket& listener,
        SecureSocket& socket,
        int timeout)
{
  TcpServerSocket::AcceptStatus ra = listener.AcceptConnection(socket, timeout);
  if (ra == TcpServerSocket::ACCEPT_SUCCESS)
  {
    SSL_set_fd(static_cast<SSL*>(socket.m_ssl), socket.m_socket);
    SSL_set_accept_state(static_cast<SSL*>(socket.m_ssl));

    /* do SSL handshake */
    int r = SSL_accept(static_cast<SSL*>(socket.m_ssl));
    if (r < 1)
    {
      socket.m_ssl_error = ERR_get_error();
      return TcpServerSocket::ACCEPT_FAILURE;
    }
    DBG(DBG_PROTO, "%s: SSL handshake initialized\n", __FUNCTION__);
    socket.m_connected = true;
  }
  return ra;
}

#else

SSLSessionFactory::SSLSessionFactory()
: m_client_ctx(nullptr)
, m_enabled(false)
{
  DBG(DBG_INFO, "%s: SSL feature is disabled\n", __FUNCTION__);
}

SSLSessionFactory::~SSLSessionFactory()
{
}

SecureSocket* SSLSessionFactory::NewClientSocket()
{
  return nullptr;
}

SSLServerContext::~SSLServerContext()
{
}

bool SSLServerContext::InitContext(const std::string& certfile, const std::string& pkeyfile)
{
  return false;
}

SecureSocket* SSLServerContext::NewServerSocket()
{
  return nullptr;
}

SecureSocket::SecureSocket(void* ssl)
: TcpSocket()
, m_ssl(ssl)
, m_cert(nullptr)
, m_connected(false)
, m_ssl_error(0)
{
}

SecureSocket::~SecureSocket()
{
}

bool SecureSocket::Connect(const char* server, unsigned port, int rcvbuf)
{
  (void)server;
  (void)port;
  (void)rcvbuf;
  return false;
}

size_t SecureSocket::ReceiveData(void* buf, size_t n)
{
  (void)buf;
  (void)n;
  return 0;
}

bool SecureSocket::SendData(const char* buf, size_t size)
{
  (void)buf;
  (void)size;
  return false;
}

void SecureSocket::Disconnect()
{
}

bool SecureSocket::IsCertificateValid(std::string& str)
{
  (void)str;
  return false;
}

const char* SecureSocket::GetSSLError()
{
  return "SSL not available";
}

TcpServerSocket::AcceptStatus SecureServerSocket::AcceptConnection(
        TcpServerSocket& listener,
        SecureSocket& socket,
        int timeout)
{
  (void)listener;
  (void)socket;
  (void)timeout;
  return TcpServerSocket::ACCEPT_ERROR;
}

#endif
