#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "go_quiet.h"
#include "obfsutil.h"

#define GO_QUIET_OPT_FMT "ServerName=%s;Key=%s;TicketTimeHint=3600;Browser=chrome"
#define TLS_WAIT_REPLY 1
#define TLS_ESTABLISHED 8
#define TLS_REPLY_NUM 3
#define realloc_cap(ptr, size) do { \
    if ((ptr ## _capacity) < (size)) { \
        (ptr) = (char *)realloc((ptr), (size) * 2); \
        (ptr ## _capacity) = (size) * 2; \
    }; \
} while (0)
#define alloc_cap(ptr, size) do { \
    if ((ptr ## _capacity) < (size)) { \
        if ((ptr) != NULL) { \
            free(ptr); \
        } \
        (ptr) = (char *)malloc((size) * 2); \
        (ptr ## _capacity) = (size) * 2; \
    }; \
} while (0)

// Exported go functions
extern void go_quiet_setopt(char *opt, int *err);
extern void go_quiet_freeopt();
extern void go_quiet_make_hello(char **data, size_t *out_len);
extern void go_quiet_make_reply(char **data, size_t *out_len);

typedef struct go_quiet_local_data {
    int handshake_status;
    int server_replied;
    char *send_buffer;
    int send_buffer_size;
    char *recv_buffer;
    int recv_buffer_size;
    char *out_buffer;
    int send_buffer_capacity;
    int recv_buffer_capacity;
    int out_buffer_capacity;
}go_quiet_local_data;

void go_quiet_local_data_init(go_quiet_local_data* local) {
    local->handshake_status = 0;
    local->server_replied = 0;
    local->send_buffer = malloc(0);
    local->send_buffer_size = 0;
    local->recv_buffer = malloc(0);
    local->recv_buffer_size = 0;
    local->out_buffer = malloc(0);
    local->send_buffer_capacity = 0;
    local->recv_buffer_capacity = 0;
    local->out_buffer_capacity = 0;
}

obfs * go_quiet_new_obfs() {
    obfs * self = new_obfs();
    self->l_data = malloc(sizeof(go_quiet_local_data));
    go_quiet_local_data_init((go_quiet_local_data*)self->l_data);
    return self;
}

void go_quiet_init(const char *plugin_name, const char *param, const char *pass) {
    if (plugin_name == NULL ||
        strcmp(plugin_name, "go_quiet") != 0) {
        return;
    }
    if (param == NULL) {
        param = "";
    }
    if (pass == NULL) {
        pass = "";
    }
    int err = 1;
    size_t plen = strlen(param);
    size_t klen = strlen(pass);
    size_t total = sizeof(GO_QUIET_OPT_FMT) + plen + klen;
    char *buf = malloc(total);
    if (buf) {
        snprintf(buf, total, GO_QUIET_OPT_FMT, param, pass);
        go_quiet_setopt(buf, &err);
        free(buf);
    }
    if (err != 0) {
        LOGE("Wrong options of go_quiet");
    }
}

void go_quiet_release()
{
    go_quiet_freeopt();
}

int go_quiet_get_overhead(obfs *self) {
    return 5;
}

void go_quiet_dispose(obfs *self) {
    go_quiet_local_data *local = (go_quiet_local_data*)self->l_data;
    if (local->send_buffer != NULL) {
        free(local->send_buffer);
        local->send_buffer = NULL;
    }
    if (local->recv_buffer != NULL) {
        free(local->recv_buffer);
        local->recv_buffer = NULL;
    }
    if (local->out_buffer != NULL) {
        free(local->out_buffer);
        local->out_buffer = NULL;
    }
    free(local);
    dispose_obfs(self);
}

void go_quiet_pack_data(char *encryptdata, int start, int len, char *out_buffer, int outlength) {
    out_buffer[outlength] = 0x17;
    out_buffer[outlength + 1] = 0x3;
    out_buffer[outlength + 2] = 0x3;
    out_buffer[outlength + 3] = (char)(len >> 8);
    out_buffer[outlength + 4] = (char)len;
    memcpy(out_buffer + outlength + 5, encryptdata + start, len);
}

int go_quiet_client_encode(obfs *self, char **pencryptdata, int datalength, size_t* capacity) {
    char *encryptdata = *pencryptdata;
    go_quiet_local_data *local = (go_quiet_local_data*)self->l_data;
    char * out_buffer = NULL;

    if (local->handshake_status == TLS_ESTABLISHED) {
        if (datalength < 1024) {
            if ((int)*capacity < datalength + 5) {
                *pencryptdata = (char*)realloc(*pencryptdata, *capacity = (size_t)((datalength + 5) * 2));
                encryptdata = *pencryptdata;
            }
            memmove(encryptdata + 5, encryptdata, datalength);
            encryptdata[0] = 0x17;
            encryptdata[1] = 0x3;
            encryptdata[2] = 0x3;
            encryptdata[3] = (char)(datalength >> 8);
            encryptdata[4] = (char)datalength;
            return datalength + 5;
        } else {
            alloc_cap(local->out_buffer, datalength + 4096);
            int start = 0;
            int outlength = 0;
            int len;
            while (datalength - start > 2048) {
                len = xorshift128plus() % 4096 + 100;
                if (len > datalength - start)
                    len = datalength - start;
                go_quiet_pack_data(encryptdata, start, len, local->out_buffer, outlength);
                outlength += len + 5;
                start += len;
            }
            if (datalength - start > 0) {
                len = datalength - start;
                go_quiet_pack_data(encryptdata, start, len, local->out_buffer, outlength);
                outlength += len + 5;
            }
            if ((int)*capacity < outlength) {
                *pencryptdata = (char*)realloc(*pencryptdata, *capacity = (size_t)(outlength * 2));
                encryptdata = *pencryptdata;
            }
            memcpy(encryptdata, local->out_buffer, outlength);
            return outlength;
        }
    }

    if (datalength > 0) {
        if (datalength < 1024) {
            realloc_cap(local->send_buffer, (size_t)(local->send_buffer_size + datalength + 5));
            go_quiet_pack_data(encryptdata, 0, datalength, local->send_buffer, local->send_buffer_size);
            local->send_buffer_size += datalength + 5;
        } else {
            out_buffer = (char*)malloc((size_t)(datalength + 4096));
            int start = 0;
            int outlength = 0;
            int len;
            while (datalength - start > 2048) {
                len = xorshift128plus() % 4096 + 100;
                if (len > datalength - start)
                    len = datalength - start;
                go_quiet_pack_data(encryptdata, start, len, out_buffer, outlength);
                outlength += len + 5;
                start += len;
            }
            if (datalength - start > 0) {
                len = datalength - start;
                go_quiet_pack_data(encryptdata, start, len, out_buffer, outlength);
                outlength += len + 5;
            }
            if ((int)*capacity < outlength) {
                *pencryptdata = (char*)realloc(*pencryptdata, *capacity = (size_t)(outlength * 2));
                encryptdata = *pencryptdata;
            }
            realloc_cap(local->send_buffer, (size_t)(local->send_buffer_size + outlength));
            memcpy(local->send_buffer + local->send_buffer_size, out_buffer, outlength);
            local->send_buffer_size += outlength;
            free(out_buffer);
        }
    }

    if (local->handshake_status == 0) {
        size_t hello_size = 0;
        go_quiet_make_hello(&out_buffer, &hello_size);
        datalength = hello_size;
        local->server_replied = 0;
        local->handshake_status = TLS_WAIT_REPLY;
    } else if (datalength == 0) {
        char *reply = NULL;
        size_t reply_size = 0;
        go_quiet_make_reply(&reply, &reply_size);
        datalength = local->send_buffer_size + reply_size;
        out_buffer = (char*)malloc((size_t)datalength);
        memcpy(out_buffer, reply, reply_size);
        memcpy(out_buffer + reply_size, local->send_buffer, local->send_buffer_size);
        free(local->send_buffer);
        free(reply);
        local->send_buffer = NULL;
        local->handshake_status = TLS_ESTABLISHED;
    } else {
        return 0;
    }
    if ((int)*capacity < datalength) {
        *pencryptdata = (char*)realloc(*pencryptdata, *capacity = (size_t)(datalength * 2));
        encryptdata = *pencryptdata;
    }
    if (out_buffer) {
        memmove(encryptdata, out_buffer, datalength);
        free(out_buffer);
    } else {
        return 0;
    }
    return datalength;
}

int go_quiet_client_decode(obfs *self, char **pencryptdata, int datalength, size_t* capacity, int *needsendback) {
    char *encryptdata = *pencryptdata;
    go_quiet_local_data *local = (go_quiet_local_data*)self->l_data;

    *needsendback = 0;
    if (local->handshake_status == TLS_ESTABLISHED) {
        local->recv_buffer_size += datalength;
        realloc_cap(local->recv_buffer, (size_t)local->recv_buffer_size);
        memcpy(local->recv_buffer + local->recv_buffer_size - datalength, encryptdata, datalength);
        datalength = 0;
        char *recv_buffer = local->recv_buffer;
        while (local->recv_buffer_size > 5) {
            if (recv_buffer[0] != 0x17)
                return -1;
            int size = ((int)(unsigned char)recv_buffer[3] << 8) + (unsigned char)recv_buffer[4];
            if (size + 5 > local->recv_buffer_size)
                break;
            if ((int)*capacity < datalength + size) {
                *pencryptdata = (char*)realloc(*pencryptdata, *capacity = (size_t)((datalength + size) * 2));
                encryptdata = *pencryptdata;
            }
            memcpy(encryptdata + datalength, recv_buffer + 5, size);
            datalength += size;
            local->recv_buffer_size -= 5 + size;
            recv_buffer += 5 + size;
        }
        if (local->recv_buffer_size > 0) {
            memmove(local->recv_buffer, recv_buffer, local->recv_buffer_size);
        }
        return datalength;
    }
    if (local->handshake_status == TLS_WAIT_REPLY) {
        local->recv_buffer_size += datalength;
        realloc_cap(local->recv_buffer, (size_t)local->recv_buffer_size);
        memcpy(local->recv_buffer + local->recv_buffer_size - datalength, encryptdata, datalength);
        datalength = 0;
        char *recv_buffer = local->recv_buffer;
        while (local->recv_buffer_size > 5) {
            int magic = recv_buffer[0];
            if (!(magic == 0x14 || magic == 0x16))
                return -1;
            int size = ((int)(unsigned char)recv_buffer[3] << 8) + (unsigned char)recv_buffer[4];
            if (size + 5 > local->recv_buffer_size)
                break;
            local->server_replied++;
            datalength += size;
            local->recv_buffer_size -= 5 + size;
            recv_buffer += 5 + size;
        }
        if (local->recv_buffer_size > 0) {
            memmove(local->recv_buffer, recv_buffer, local->recv_buffer_size);
        }
        if (local->server_replied == TLS_REPLY_NUM) {
            *needsendback = 1;
            return 0;
        }
    }
    return -1;
}

#undef realloc_cap
#undef alloc_cap
