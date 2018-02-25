#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <dirent.h>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "myftp.h"
#include "myssl.h"

#define SERVER_CERT "server_cert.pem"
#define SERVER_KEY "server_key.pem"
#define SERVER_PASS "server"

#define MAX_THREAD_NUM 2048
#ifdef DIR_64
  #define dirent dirent64
  #define readdir readdir64
#endif

typedef struct threadargs{
  int client_sd;
  SSL* ssl;
} threadargs;

void* handle_thread(void* args);

int main(int argc, char** argv){
    int PORT = atoi(argv[1]);
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    long val = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1){
        _printf(sd, "Setsockopt error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(1);
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    if(bind(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        _printf(sd, "Bind error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    
    if(listen(sd, 3) < 0){
        _printf(sd, "Listen error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    _printf(sd, "Server listening on port %d ...\n", PORT);
    
    	SSL* ssl;
	SSL_CTX* ctx;
	const SSL_METHOD* meth = SSLv23_method();
	if(meth== NULL) fprintf(stderr, "ERR: Method null!\n");
    	ctx = SSL_create_ctx(meth);
	
	SSL_verify_priv_key(ctx, SERVER_CERT, SERVER_KEY, SERVER_PASS);	
	handle_err(NULL, SSL_CTX_load_verify_locations(ctx, CA_CERT, NULL));
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
	SSL_CTX_set_verify_depth(ctx, 1);

    pthread_t thread[MAX_THREAD_NUM];
    threadargs args[MAX_THREAD_NUM];
    int client_sd, t = 0;
    struct sockaddr_in client_addr;
    unsigned int client_addr_len = sizeof(client_addr);
    while((client_sd = accept(sd, (struct sockaddr*)&client_addr, &client_addr_len)) >= 0){
	SSL* ssl = SSL_new(ctx);
	if (ssl == NULL) {
		fprintf(stderr, "ERR: unable to create the ssl structure\n");
		exit(-1);
	}
	  
	SSL_set_fd(ssl, client_sd);
	handle_err(ssl, SSL_accept(ssl));
	log_peer_certificate(client_sd, ssl);
	_printf(sd, "SSL connection using %s\n", SSL_get_cipher(ssl));
	
	args[t].ssl = ssl;
        args[t].client_sd = client_sd;
        pthread_create(&thread[t], NULL, handle_thread, &args[t]);
        t++;
    }
    
    _printf(sd, "Accept error: %s (Errno:%d)\n", strerror(errno), errno);
    return 0;
}
int list_dir(const void* path, char* buffer){
	DIR* dir;
	struct dirent* direntp;
	if((dir = opendir(path)) == NULL){
		_printf(0, "Open directory %s error: %s (Errno:%d)", path, strerror(errno), errno);
		return -1;	
	}
  
  // Read directory
	int len = 0;
	while((direntp = readdir(dir)) != NULL){
		sprintf(buffer, "%s\n", direntp->d_name);
		buffer += strlen(direntp->d_name)+1;
		len += strlen(direntp->d_name)+1;
	}
	buffer[len] = '\0';
  
	if(closedir(dir) != 0){
		_printf(0, "Close directory %s error: %s (Errno:%d)", path, strerror(errno), errno);
	};
	return len;
}

void* handle_thread(void* args){
  int client_sd = ((threadargs*)args)->client_sd;
  SSL* ssl = ((threadargs*)args)->ssl;
  
  // Receive initial request header
  message_s* req_header = create_message(0, 0);
  recv_header(ssl, req_header);
  
  // Handle request
  if(req_header->type == LIST_REQUEST){
      _printf(client_sd, "Connection established with client_%d -- list\n", client_sd);
      char* buff = (char*)malloc(sizeof(char)*CHUNK_SIZE);
      unsigned int payload_len = list_dir("data", buff);
      
      // Send LIST_REPLY with "ls" result length
      message_s* res_header = create_message(LIST_REPLY, 10 + payload_len);
      send_header(ssl, res_header);
      
      // Send "ls" result
      send_data(ssl, buff, payload_len);
      free(buff);
  }else if(req_header->type == GET_REQUEST){
      _printf(client_sd, "Connection established with client_%d -- get\n", client_sd);
      // Receive file name
      int fname_len = req_header->length-10;
      char* fname = (char*)malloc(sizeof(char)*fname_len);
      recv_data(ssl, fname, fname_len);
      
      // Prepend directory name and read file
      char p[256] = "data/"; strcat(p, fname);
      char* buff = NULL;
      int payload_len = read_data(p, &buff);
      
      // Send GET_REPLY header to signal whether file exists
      if(payload_len < 0){
          message_s* res_header = create_message(GET_REPLY_NOT_EXIST, 10);
          send_header(ssl, res_header);
      }else{
          message_s* res_header = create_message(GET_REPLY, 10);
          message_s* fd_header = create_message(FILE_DATA, 10 + payload_len);
          send_header(ssl, res_header);
          
          // Send FILE_DATA with file size
          send_header(ssl, fd_header);
          
          // Send data
          send_data(ssl, buff, payload_len);
      }
      free(buff);
      free(fname);
  }else if(req_header->type == PUT_REQUEST){
      _printf(client_sd, "Connection established with client_%d -- put\n", client_sd);
      // Receive file name
      int fname_len = req_header->length-10;
      char* fname = (char*)malloc(sizeof(char)*fname_len);
      recv_data(ssl, fname, fname_len);
      
      // Send PUT_REPLY header and receive FILE_DATA with file size
      message_s* res_header = create_message(PUT_REPLY, 10);
      message_s* fd_header = create_message(0, 0);
      send_header(ssl, res_header);
      recv_header(ssl, fd_header);
      
      // Prepend directory name and receive data
      char p[256] = "data/"; strcat(p, fname);
      int payload_len = fd_header->length-10;
      char* buff = (char*) malloc(sizeof(char)*payload_len);
      recv_data(ssl, buff, payload_len);
      
      // Write data to file
      int len = write_data(p, buff, payload_len);
      _printf(client_sd, "Result (get):\nWritten %d bytes to %s!\n", len, p);
      free(buff);
      free(fname);
  }

  while(SSL_shutdown(ssl) == 0){}
  if(close(client_sd)<0) { perror("Close socket error"); exit(-1); }
  SSL_free(ssl);
  pthread_exit(NULL);
}

