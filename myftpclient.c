#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "myftp.h"

int main(int argc, char** argv){
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));
    if(connect(sd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        _printf(sd, "Connection error: %s (Errno:%d)\n", strerror(errno), errno);
        exit(0);
    }
    
    _printf(sd, "Connection established with %s:%s -- %s\n", argv[1], argv[2], argv[3]);
    if(strcmp(argv[3], "list") == 0){
        message_s* req_header = create_message(LIST_REQUEST, 10);
        message_s* res_header = create_message(0, 0);
        send_header(sd, req_header);
        recv_header(sd, res_header);
        int payload_len = res_header->length-10;
        char* buff = (char*) malloc(sizeof(char)*payload_len);
        _printf(sd, "[LIST] Result contains %d bytes.\n", payload_len);
        recv_data(sd, buff, payload_len);
        _printf(sd, "Result (list):\n%s\n", buff);
        free(buff);
    }else if(strcmp(argv[3], "get") == 0){
        message_s* req_header = create_message(GET_REQUEST, 10+strlen(argv[4])+1);
        message_s* res_header = create_message(0, 0);
        send_header(sd, req_header);
        send_data(sd, argv[4], strlen(argv[4])+1);
        
        recv_header(sd, res_header);
        
        if(res_header->type == GET_REPLY){
            message_s* fd_header = create_message(0, 0);
            recv_header(sd, fd_header);
            
            int payload_len = fd_header->length-10;
            char* buff = (char*) malloc(sizeof(char)*payload_len);
            
            _printf(sd, "[GET] %s contains %d bytes.\n", argv[4], payload_len);
            recv_data(sd, buff, payload_len);
            
            int len = write_data(argv[4], buff, payload_len);
            _printf(sd, "Result (get):\nWritten %d bytes to %s!\n", len, argv[4]);
            
            free(buff);
        }else{
            _printf(sd, "File does not exist!\n");
        }
    }else if(strcmp(argv[3], "put") == 0){
        char* buff = NULL;
        int payload_len = read_data(argv[4], &buff);
        if(payload_len < 0){ exit(0); }
        
        message_s* req_header = create_message(PUT_REQUEST, 10+strlen(argv[4])+1);
        message_s* res_header = create_message(0, 0);
        send_header(sd, req_header);
        send_data(sd, argv[4], strlen(argv[4])+1);
        
        recv_header(sd, res_header);
        
        if(res_header->type == PUT_REPLY){
            _printf(sd, "[PUT] %s contains %d bytes.\n", argv[4], payload_len);
            message_s* fd_header = create_message(FILE_DATA, 10 + payload_len);
            send_header(sd, fd_header);
            send_data(sd, buff, payload_len);
        }
        free(buff);
    }
    
    return 0;
}