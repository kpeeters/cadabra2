
.PHONY: docker

# Prepare a tarball with 
docker-prepare:
	(git archive --format=tar.gz master > docker/master.tar.gz)

docker-build:
	docker build -t kpeeters/cadabra2:master --force-rm docker

docker-run:
	docker run -t -i kpeeters/cadabra2:master /bin/bash

