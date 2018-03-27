all: .libs/libgoquiet.a

.libs/%.a: %.go
	type go && \
	CGO_ENABLED=1 GOPATH="$(abs_top_srcdir)/goquiet" @GOCROSS@ \
	go build @GOFLAGS@ -o $@ -v -buildmode=c-archive $<

clean:
	rm -Rf .libs
