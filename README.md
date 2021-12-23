# Overview
The Voxxlr processor converts input files into a format required by the voxxlr doc server. The pipeline is deployed
using and thus requied the docker engine to be installed. Once that is done follow the instructions below to build and 
execute the processor. 

## building the docker image

Run the following shell script to build the docker image.

```sh
./docker-build.sh
```

Once done a new docker image called vxProcessor should be listed when running 

```sh
docker images
```

## running the docker image

Run the following shell script 

```sh
./docker-run.sh DATA_DIR
```

to process datasets located in the DATA_DIR directory. The DATA_DIR must contain a file call process.yaml that
contains instructions for the processor including which files to process. 




