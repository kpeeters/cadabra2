
all:
	@echo -n "To build Cadabra, \n\n   mkdir build\n   cd build\n   cmake ..\n   make\n\nIf you need help, email info@cadabra.science\n"

tarball:
	git archive --format=tar --prefix=cadabra2-latest/ HEAD | gzip > ${HOME}/tmp/cadabra2-latest.tar.gz

doc:
	doxygen

webup:
	cd build; make -f web2/Makefile
	cd web2/cadabra2; clay build; rsync -avz --chmod=+rx build/ cadabra_web:/var/www/cadabra2/;  rsync -avz --chmod=+rx source/static/styles/ cadabra_web:/var/www/cadabra2/static/styles; rsync -avz --chmod=+rx source/static/images/ cadabra_web:/var/www/cadabra2/static/images/
