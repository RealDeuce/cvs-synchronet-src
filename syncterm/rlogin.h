#ifndef _RLOGIN_H_
#define _RLOGIN_H_

#include <sockwrap.h>

extern SOCKET	rlogin_socket;
extern int	rcvtimeo;

int rlogin_recv(char *buffer, size_t buflen);
int rlogin_send(char *buffer, size_t buflen, unsigned int timeout);
int rlogin_connect(char *addr, int port, char *ruser, char *passwd, int dumb);
int rlogin_close(void);

#endif
