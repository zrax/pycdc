FROM python:3.10-buster

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt -y install cmake build-essential && rm -rf /var/lib/apt/lists/*

COPY . /usr/src/pycdc
RUN cd /usr/src/pycdc && cmake . && make && make install
