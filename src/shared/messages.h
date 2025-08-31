#ifndef MESSAGES
#define MESSAGES

typedef struct login_email_check_request {

} LoginEmailCheckRequest

typedef union data {

	LoginEmailCheckRequest login_email_check_request;
} Data;

typedef struct message {

	uint32_t msg_id;

} Message;

#endif 
