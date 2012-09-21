#ifndef __MINISIP_
#define __MINISIP_

typedef enum SIP_STATUS_T {

	SIP_INIT,
	SIP_REGISTER,
	SIP_100_TRYING,
	SIP_401_UNAUTH,
	SIP_200_OK,
	SIP_OPTIONS,
	SIP_INCOMMING_CALL,
	SIP_OUTING_CALL,
	SIP_HUNGING,

} SIP_STATUS_T;

typedef struct {

	SIP_STATUS_T status;
	char *status_char;

	char *via;
	char *via_rport;
	char *via_rcvd;
	char *via_branch;

	char *from;
	char *from_tag;
	char *to;
	char *to_tag;

	char *cseq_num;
	char *cseq_method;
	char *allow;
	
	char *accept;
	char *accept_version;

	char *supported;
	char *allow_events;
	char *content_length;

	char *call_id;
	char *user_agent;
	char *contact;

	char *expires;
	char *max_forwards;
	
	char *www_auth;
	char *www_auth_realm;
	char *www_auth_nonce;

	char *date;

} SIP_MSG_T;

typedef struct {
	int sd;

	// account information
	char user_name[32];
	char user_pawd[32];
	char local_ip[32];

	// server information
	char server_ip[32];
	//struct sockaddr_in sevr_addr;

	//session information
	SIP_MSG_T msg;

} SIP_T;

int sip_init(SIP_T *sip);
int sip_str_to_msg(SIP_MSG_T *msg, const char *data);
void sip_msg_to_str(SIP_T *sip, char *buf);

#endif
