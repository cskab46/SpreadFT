#define PAYLOAD 1200

typedef struct message_struct{
	int process_index;
	int message_index;
	int random_number;
	char data[PAYLOAD];
} message;
