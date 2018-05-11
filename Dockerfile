FROM python:3.6.5-alpine as kipy-builder
RUN apk add --no-cache gcc g++ libc-dev make cmake
RUN pip install Cython
COPY . /
RUN python setup.py install
RUN pip uninstall Cython

FROM python:3.6.5-alpine
RUN apk add --no-cache libstdc++
COPY --from=kipy-builder \
  /usr/local/lib/python3.6/site-packages/ \
  /usr/local/lib/python3.6/site-packages/
