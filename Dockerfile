# A simple Ubuntu container with OPA, uses `v0.41.0`.
# https://github.com/open-policy-agent/opa/releases
# To test: `docker run $(docker build -q .) version`.

FROM ubuntu:latest

RUN apt-get update -y
RUN apt-get install -y wget curl
RUN apt-get install -y jq

# NOTE(dkorolev): `cpp` is the C preprocessor, that is very lightweight, not the full C++ dev env.
RUN apt-get install -y cpp

RUN wget https://github.com/open-policy-agent/opa/releases/download/v0.41.0/opa_linux_amd64
RUN mv opa_linux_amd64 opa
RUN sha256sum opa
RUN sha256sum opa | cut -f1 -d' ' >opa.sha256
RUN echo "a1be8ea092a965e9a659654b482666b1ae2bbca5c758ec0d2405fab313481a74" >opa.sha256.golden
RUN if ! diff opa.sha256 opa.sha256.golden ; then echo 'OPA binary SHA256 check failed.' ; exit 1 ; fi
RUN chmod +x opa
RUN mv opa /usr/bin/opa

COPY src /src

# NOTE(dkorolev): Need to run `npm i` before building this Docker container.
COPY node_modules /node_modules

ENTRYPOINT ["/src/docker_entrypoint.sh"]
