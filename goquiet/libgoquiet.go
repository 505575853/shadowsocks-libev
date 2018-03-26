package main

import (
	"time"

	"github.com/cbeuw/GoQuiet/gqclient"
	"github.com/cbeuw/GoQuiet/gqclient/TLS"
)

/*
#include <stdlib.h>
#include <string.h>

//extern void go_quiet_setopt(char *opt, int *err);
//extern void go_quiet_make_hello(char **data, size_t *out_len);
//extern void go_quiet_make_reply(char **data, size_t *out_len);
*/
import "C"

var sta *gqclient.State = nil

func make_c_array(array []byte, data **C.char, out_len *C.size_t) {
	*data = (*C.char)(C.CBytes(array))
	*out_len = C.size_t(len(array))
}

//export go_quiet_setopt
func go_quiet_setopt(c_opt *C.char, c_err *C.int) {
	if c_err == nil {
		return
	}
	*c_err = 1
	if c_opt == nil {
		return
	}
	opt := C.GoString(c_opt)
	opaque := gqclient.BtoInt(gqclient.CryptoRandBytes(32))
	sta = &gqclient.State{
		SS_LOCAL_HOST:  "",
		SS_LOCAL_PORT:  "",
		SS_REMOTE_HOST: "",
		SS_REMOTE_PORT: "",
		Now:            time.Now,
		Opaque:         opaque,
	}
	err := sta.ParseConfig(opt)
	if err != nil {
		return
	}
	sta.SetAESKey()
	*c_err = 0
}

//export go_quiet_make_hello
func go_quiet_make_hello(data **C.char, out_len *C.size_t) {
	if sta == nil {
		*out_len = 0
		*data = nil
		return
	}
	make_c_array(TLS.ComposeInitHandshake(sta), data, out_len)
}

//export go_quiet_make_reply
func go_quiet_make_reply(data **C.char, out_len *C.size_t) {
	make_c_array(TLS.ComposeReply(), data, out_len)
}

func main() {}
