
all:
	@echo -n "To build Cadabra, \n\n   mkdir build\n   cd build\n   cmake ..\n   make\n\nIf you need help, email info@cadabra.science\n"

tarball:
	git archive --format=tar --prefix=cadabra2-latest/ HEAD | gzip > ${HOME}/tmp/cadabra2-latest.tar.gz

doc:
	doxygen
