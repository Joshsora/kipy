FROM python:3.6.5-alpine as kipy-builder
RUN  apk add --update gcc g++ libc-dev make cmake
COPY . /
RUN python setup.py install

FROM python:3.6.5-alpine as kipy
RUN  apk add --update libstdc++
COPY --from=kipy-builder \
  /usr/local/lib/python3.6/site-packages/ \
  /usr/local/lib/python3.6/site-packages/
