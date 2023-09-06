# A simple Alpine container with OPA `v0.41.0` and Asbyrgi addons.
# OPA downloaded from: https://github.com/open-policy-agent/opa/releases
# To test: `docker run $(docker build -q .) version`.

FROM alpine:latest

RUN apk add bash curl ucpp jq nodejs

RUN wget https://github.com/open-policy-agent/opa/releases/download/v0.41.0/opa_linux_amd64_static
RUN mv opa_linux_amd64_static opa
RUN chmod +x opa
RUN sha256sum opa
RUN sha256sum opa | cut -f1 -d' ' >opa.sha256
RUN echo "acdcfcfaace76588c2c3aa6a59aebf53e5906d019058d6a963e57b02c68b755b" >opa.sha256.golden
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
