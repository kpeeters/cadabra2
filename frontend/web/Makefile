
.PHONY: docker

# Prepare a tarball with 
docker-prepare:
	(cd ../..; git archive --format=tar.gz --prefix=cadabra2/ web > frontend/web/docker/master.tar.gz)

docker-build:
	docker build -t kpeeters/cadabra2:master --force-rm docker

docker-run:
	docker run -t -i --expose=80 -P kpeeters/cadabra2:master /bin/bash

