
.PHONY: docker

docker-prepare:
	(cd docker; wget https://github.com/kpeeters/cadabra2/archive/2.2.7.tar.gz)

docker-build:
	docker build -t kpeeters/cadabra2:2.2.7 --force-rm docker

docker-run:
	docker run -t -i kpeeters/cadabra2:2.2.7 /bin/bash

