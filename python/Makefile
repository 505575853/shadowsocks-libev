MINGW-PREFIX = i686-w64-mingw32

all: pack

server.exe:
	LDFLAGS="-s" python nuitka-bin.py --clang --recurse-to=yogurt --python-version=2.7 --show-scons --remove-output --show-progress --file-reference-choice=runtime server.py

nuitka:
	python nuitka-bin.py --standalone --python-version=2.7 --show-scons --remove-output --show-progress --file-reference-choice=runtime server.py

siphashc/siphashc.so:
	cd siphashc; python setup.py test

winselect/wepoll.lib:
	$(MINGW-PREFIX)-gcc winselect/wepoll.c -c -o winselect/wepoll.o -O2 -Wall
	$(MINGW-PREFIX)-gcc winselect/chkstk.S -c -o winselect/chkstk.o -O2 -Wall
	$(MINGW-PREFIX)-ar rcs winselect/wepoll.lib winselect/*.o

pack:
	cp windows/pylib.zip windows/python27.zip
	cd windows; zip -ur python27.zip winselect.pyd win_inet_pton.py
	find yogurt -iname '*.pyc' -delete
	zip -ur windows/python27.zip yogurt server.py

git:
	git archive -o python.tar.gz --prefix=python/ HEAD

ss-server: server.exe siphashc/siphashc.so
	mv server.exe $@
	mv siphashc/siphashc.so .

clean:
	rm -f ss-server siphashc.so
	rm -Rf siphashc/siphashc.so siphashc/build
	rm -Rf winselect/wepoll.lib winselect/build
	rm -f windows/python27.zip

.PHONY: clean all nuitka pack git
