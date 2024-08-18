# Decompyle++ in Docker

## build

```sh
docker build -t pycdc:latest .
```

## run

```sh
cd /samples/directory

docker run --rm -ti -v `pwd`:/data/ pycdc:latest pycdc /data/sample.pyc
# (or)
docker run --rm -ti -v `pwd`:/data/ pycdc:latest pycdas /data/sample.pyc
```
