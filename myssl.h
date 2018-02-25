#define RSA_CLIENT_CA_CERT  "cacert.pem"
#define VERIFY_CLIENT  0
#define ENABLE_SSL 1

SSL_CTX* SSL_create_ctx(void);
SSL* SSL_request_handshake(int sd, SSL_CTX* ctx);
SSL* SSL_respond_handshake(int sd, SSL_CTX* ctx);
void SSL_cleanup(int sd, SSL* ssl, SSL_CTX* ctx);