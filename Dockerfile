FROM python:3.6.5-alpine as kipy-builder
RUN apk add --no-cache gcc g++ libc-dev zlib-dev make cmake
ENV LIBRARY_PATH=/lib:/usr/lib
COPY . /
RUN python setup.py install
RUN pip install -r requirements.txt

FROM python:3.6.5-alpine
RUN apk add --no-cache libstdc++ zlib-dev
COPY --from=kipy-builder \
  /usr/local/lib/python3.6/site-packages/ \
  /usr/local/lib/python3.6/site-packages/
