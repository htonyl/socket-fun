#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "myftp.h"
#include "myssl.h"

void handle_err(SSL* ssl, int err){
  int r;
  if (err <= 0) {
    _printf(0, "Return = %d SSL get error code = %d\n", err, SSL_get_error(ssl, r));
    perror("\tERR");
    ERR_print_errors_fp(stderr);
    exit(-1);
  }
}  


SSL_CTX* SSL_create_ctx(const SSL_METHOD* meth){
  	/* Register all algorithm */
  	/* Load encryption & hashing algorithms for the SSL program */
  	/* Load the error strings for SSL & CRYPTO APIs */
	OpenSSL_add_all_algorithms();
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_ciphers();

	/* Create an SSL_METHOD structure (choose an SSL/TLS protocol version) */
	if (meth == NULL){
		fprintf(stderr, "ERR: Unable to cerate method\n");
		exit(-1);
	}

	/* Create an SSL_CTX structure */
	SSL_CTX* ctx = SSL_CTX_new(meth);
	if (ctx == NULL) {
		fprintf(stderr, "ERR: Unable to create ctx\n");
		exit(-1);
	}
  return ctx;
}

void SSL_verify_priv_key(SSL_CTX* ctx, const char cert[], const char key[], const char pass[]){
	// Passphrase is required to verify private key
	SSL_CTX_set_default_passwd_cb_userdata(ctx, (char*)pass);	
	handle_err(NULL, SSL_CTX_use_certificate_file(ctx, cert, SSL_FILETYPE_PEM));
	handle_err(NULL, SSL_CTX_use_PrivateKey_file(ctx, key, SSL_FILETYPE_PEM));
	handle_err(NULL, SSL_CTX_check_private_key(ctx));
}

SSL* SSL_request_handshake(int sd, SSL_CTX* ctx){
	/* An SSL structure is created */
	SSL* ssl = SSL_new(ctx);
	if (ssl == NULL) {
		fprintf(stderr, "ERR: cannot create ssl structure\n");
		exit(-1);
	}

 	_printf(sd, "SSL handshake starts\n");
	SSL_set_fd(ssl, sd);
	handle_err(ssl, SSL_connect(ssl));
	_printf (sd, "SSL connection using %s\n", SSL_get_cipher (ssl));
  return ssl;
}

SSL* SSL_respond_handshake(int sd, SSL_CTX* ctx){
  /* ----------------------------------------------- */
  /* TCP connection is ready. */
  /* A SSL structure is created */
  SSL* ssl = SSL_new(ctx);
  if (ssl == NULL) {
    fprintf(stderr, "ERR: unable to create the ssl structure\n");
    exit(-1);
  }
  SSL_set_fd(ssl, sd);
  handle_err(ssl, SSL_accept(ssl));

  _printf(sd, "SSL connection using %s\n", SSL_get_cipher(ssl));
  return ssl;
}

/*--------------- SSL closure ---------------*/
void SSL_cleanup(int sd, SSL* ssl, SSL_CTX* ctx){
  /* Shutdown the client side of the SSL connection */
  int err;
  err = SSL_shutdown(ssl);
  handle_err(ssl, err);

  err = close(sd);
  handle_err(ssl, err);

  SSL_free(ssl);
  SSL_CTX_free(ctx);
}

void log_peer_certificate(int sd, SSL* ssl){
  	char* str;
	X509* cert = SSL_get_peer_certificate(ssl);    
	if (cert != NULL) {
		_printf (sd, "Peer Certificate:\n");
		str = X509_NAME_oneline(X509_get_subject_name(cert),0,0);
    		if (str == NULL) { exit(-1); }
		_printf (sd, "\tSubject: %s\n", str);
		free (str);

		str = X509_NAME_oneline(X509_get_issuer_name(cert),0,0);
		if (str == NULL) { exit(-1); }
		_printf (sd, "\tIssuer: %s\n", str);
		free(str);
    
		X509_free(cert);
	} else {
		printf("Peer certificate does not exist.\n");
	}
}
