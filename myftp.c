#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>
#include "myftp.h"

pthread_mutex_t WRITE_MUTEX = PTHREAD_MUTEX_INITIALIZER;

char* info_log(int sd){
	char* output = (char*)malloc(sizeof(char)*128);
  struct tm * timeinfo;
	time_t rawtime;
	time ( &rawtime );	
	timeinfo = localtime ( &rawtime );
	sprintf(output, "%02d:%02d:%02d |- [client: %d]", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, sd);
	return output;
}

void _printf(int sd, const char* format, ...){
  // Prepend info log to all stdout outputs
  va_list args;
  va_start (args, format);
  printf("%s: ", info_log(sd));
  vprintf(format, args);
  va_end (args);  
}

message_s* create_message(unsigned char type, unsigned int length){
  message_s* m = (message_s*)malloc(sizeof(message_s));
  memcpy(m->protocol, "myftp", 5);
  m->type = type;
  m->length = length;
  return m;
}

int _send(int sd, void* data, unsigned int data_len){
  int len;
  if((len=send(sd, data, data_len, 0)) < 0){
      _printf(sd, "Send error: %s (Errno:%d)\n", strerror(errno), errno);
      exit(0);
  }
  return len;
}

int _recv(int sd, void* data, unsigned int data_len){
  int len;
  if((len=recv(sd, data, data_len, 0)) < 0){
      _printf(sd, "Recv error: %s (Errno:%d)\n", strerror(errno), errno);
      exit(0);
  }
  return len;
}

void send_header(int sd, message_s* header){
  // Force header to be encoded big endian for transfer
  header->length = htonl(header->length);
  int len = _send(sd, header, sizeof(*header));
}

void recv_header(int sd, message_s* header){
  // Encode header little endian from transfer
  int len = _recv(sd, header, sizeof(*header));
  header->length = ntohl(header->length);
}

void send_data(int sd, void* data, unsigned int data_len){
  int len, i;
  // Loop send until sent bytes equals data size
  for(i=0; i<data_len;){
    len = _send(sd, data + i, i + CHUNK_SIZE < data_len ? CHUNK_SIZE : data_len - i);
    i += len;
    _printf(sd, "Sent %.1f%% (%d/%d bytes)\r", (double)i/data_len*100, i, data_len);
  }
  printf("\n");
}

void recv_data(int sd, void* data, unsigned int data_len){
  int len, i;
  // Loop receive until received bytes equals data size
  for(i=0; i<data_len;){
    len = _recv(sd, data + i, i + CHUNK_SIZE < data_len ? CHUNK_SIZE : data_len - i);
    i += len;
    _printf(sd, "Received %.1f%% (%d/%d bytes)\r", (double)i/data_len*100, i, data_len);
  }
  printf("\n");
}

int write_data(char path[], char* buff, int buff_len){
  // Handle uncreated directory 
  char* fname = basename(path);
  char* dname = dirname(path);
  DIR* dir = opendir(dname);
  if (dir){ closedir(dir); }
  else {
    _printf(0, "Creating directory %s/\n", dname);
    mkdir(dname, 0700);
  }
  
  // Open file
  char p[256]; sprintf(p, "%s/%s", dname, fname);
  FILE *file = fopen(p, "wb");  
  if (file == NULL){
    _printf(0, "Open file %s error: %s (Errno:%d)\n", path, strerror(errno), errno);
    return -1;
  }
  
  _printf(0, "Writing %d bytes to file %s\n", buff_len, p);
  setvbuf(file, NULL, _IONBF, 0);
  
  // Buffers are shared between threads, lock to prevent unwanted write
  pthread_mutex_lock(&WRITE_MUTEX);
  size_t nwritten = fwrite(buff, 1, buff_len, file);
  pthread_mutex_unlock(&WRITE_MUTEX);
  
  fclose(file);
  return nwritten;
}

int read_data(char path[], char** buff_ptr){
  // Open file
  FILE *file = fopen(path, "rb");  
  if (file == NULL){
    _printf(0, "Open file %s error: %s (Errno:%d)\n", path, strerror(errno), errno);
    return -1;
  }
  
  // Seek file size
  fseek(file, 0, SEEK_END); int fsize = ftell(file); rewind(file);
  
  // Read data from file
  char* data = (char*)malloc(sizeof(char)*(fsize+1));
  size_t nread = fread(data, 1, sizeof(char)*fsize, file);
  data[fsize] = '\0';
  
  // Check file size matches read data size
  if(nread != fsize){ free(data); return -1; }
  *buff_ptr = data; fclose(file);
  return fsize;
}