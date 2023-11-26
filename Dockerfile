# A simple Alpine container with OPA `v0.58.0` and Asbyrgi addons.
# OPA downloaded from: https://github.com/open-policy-agent/opa/releases
# To get OPA version: `docker run $(docker build -q .) version`.
# To get Asbyrgi version of a GHCR image: `docker run ghcr.io/dkorolev/asbyrgi asbyrgi_version`.

FROM alpine:latest

RUN apk add bash curl ucpp jq nodejs

RUN wget https://github.com/open-policy-agent/opa/releases/download/v0.58.0/opa_linux_amd64_static
RUN mv opa_linux_amd64_static opa
RUN chmod +x opa
RUN sha256sum opa
RUN sha256sum opa | cut -f1 -d' ' >opa.sha256
RUN echo "7bb75b14c9bcb5798d42bed5fc45c438ee5bb783894733ce553ba3445f66034f" >opa.sha256.golden
RUN if ! diff opa.sha256 opa.sha256.golden ; then echo 'OPA binary SHA256 check failed.' ; exit 1 ; fi
RUN mv opa /usr/local/bin

RUN apk add gradle curl

RUN curl -sSLO https://github.com/pinterest/ktlint/releases/download/0.50.0/ktlint
RUN ls -las ktlint
RUN mkdir -p /usr/local/bin/ && chmod a+x ktlint && mv ktlint /usr/local/bin/

COPY ./kt_test_golden /kt_test
RUN tar czvf kt_test.tar.gz kt_test

COPY src /src

ENTRYPOINT ["/src/docker_entrypoint.sh"]
