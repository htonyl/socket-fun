#define LIST_REQUEST		0xA1
#define LIST_REPLY			0xA2
#define GET_REQUEST			0xB1
#define GET_REPLY			0xB2
#define GET_REPLY_NOT_EXIST	0xB3
#define PUT_REQUEST			0xC1
#define PUT_REPLY			0xC2
#define FILE_DATA			0xFF
#define CHUNK_SIZE		  524288

typedef struct message_s {
	unsigned char protocol[5]; // protocol string (5 bytes)
	unsigned char type;		   // type (1 byte)
	unsigned int length;	   // length (4 bytes)
}__attribute__((packed)) message_s;

char* info_log(int sd);
void _printf(int sd, const char* format, ...);
message_s* create_message(unsigned char type, unsigned int length);
void send_header(SSL* ssl, message_s* header);
void recv_header(SSL* ssl, message_s* header);
void send_data(SSL* ssl, void* data, unsigned int data_len);
void recv_data(SSL* ssl, void* data, unsigned int data_len);
int write_data(char path[], char* buff, int buff_len);
int read_data(char path[], char** buff_ptr);
