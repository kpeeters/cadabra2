
all:
	@echo -n "\nTo build Cadabra, \n\n   mkdir build\n   cd build\n   cmake ..\n   make\n\nThe other targets here are (for maintainer purposes only)\n\n   tarball:     build a tarball cadabra2-latest.tar.gz of current HEAD\n   doc:         generate doxygen docs in doc\n   webup:       build web pages/tutorials/man pages and upload to server\n   updatesnoop: sync snoop repo\n   packages:    create deb/rpm packages on buildbot\n\nIf you need help, email info@cadabra.science\n\n"

tarball:
	git archive --format=tar --prefix=cadabra2-latest/ HEAD | gzip > ${HOME}/tmp/cadabra2-latest.tar.gz

doc:
	doxygen

webup:
	cd build; make -f web2/Makefile
	cd web2/cadabra2; clay build; rsync -avz --chmod=+rx build/ cadabra_web:/var/www/cadabra2/;  rsync -avz --chmod=+rx source/static/styles/ cadabra_web:/var/www/cadabra2/static/styles; rsync -avz --chmod=+rx source/static/images/ cadabra_web:/var/www/cadabra2/static/images/; rsync -avz --chmod=+rx source/static/icons/ cadabra_web:/var/www/cadabra2/static/icons/; rsync -avz --chmod=+rx source/static/pdf/ cadabra_web:/var/www/cadabra2/static/pdf/; rsync -avz --chmod=+rx source/static/scripts/ cadabra_web:/var/www/cadabra2/static/scripts/

packages:
	bash config/buildbot.sh

updatesnoop:
	cp ../snoop/src/Snoop.cc ../snoop/src/SnoopPrivate.hh ../snoop/src/Snoop.hh client_server/
