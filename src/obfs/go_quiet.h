#ifndef _GO_QUIET_H
#define _GO_QUIET_H

#ifdef GOQUIET
#include "obfs.h"

obfs * go_quiet_new_obfs();
void go_quiet_dispose(obfs *self);

int go_quiet_client_encode(obfs *self, char **pencryptdata, int datalength, size_t* capacity);
int go_quiet_client_decode(obfs *self, char **pencryptdata, int datalength, size_t* capacity, int *needsendback);

int go_quiet_get_overhead(obfs *self);
void go_quiet_init(const char *plugin_name, const char *param, const char *pass);
#endif

#endif // _GO_QUIET_H
