FROM python:3.6.5-alpine as kipy-builder
COPY . /
RUN apk add --update gcc g++ libc-dev make cmake
RUN pip install Cython
RUN python setup.py install

FROM python:3.6.5-alpine
RUN apk add --no-cache libstdc++
COPY --from=kipy-builder \
  /usr/local/lib/python3.6/site-packages/ \
  /usr/local/lib/python3.6/site-packages/
