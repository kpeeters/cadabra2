
all:
	@echo -n "To build Cadabra, \n\n   mkdir build\n   cd build\n   cmake ..\n   make\n\nIf you need help, email info@cadabra.science\n"

tarball:
	git archive --format=tar --prefix=cadabra-latest/ HEAD | gzip > ${HOME}/tmp/cadabra-latest.tar.gz

doc:
	doxygen
