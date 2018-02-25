#define RSA_CLIENT_CA_CERT  "ca_cert.pem"
#define CA_CERT "ca_cert.pem"
#define VERIFY_CLIENT  0
#define ENABLE_SSL 1

void handle_err(SSL* ssl, int err);
SSL_CTX* SSL_create_ctx(const SSL_METHOD* meth);
void SSL_verify_priv_key(SSL_CTX* ctx, const char cert[], const char key[], const char pass[]);
SSL* SSL_request_handshake(int sd, SSL_CTX* ctx);
SSL* SSL_respond_handshake(int sd, SSL_CTX* ctx);
void SSL_cleanup(int sd, SSL* ssl, SSL_CTX* ctx);
void log_peer_certificate(int sd, SSL* ssl);
