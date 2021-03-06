# Overview
The Voxxlr _processor_ converts input files into a format required by the Voxxlr [_doc_ server](https://github.com/voxxlr/doc). It is deployed
using docker and thus requires that the docker engine is already installed. 

## Building the docker image

After cloning this repos run the following commands to build the docker image.

```sh
cd processor
./docker-build.sh
```

A new docker image called __vx-processor__ should now be listed when running 

```sh
docker images
```

## Running the processor

Run the following shell script 

```sh
./docker-run.sh DATA_DIR
```

to process datasets located in the DATA_DIR directory. It must also contain a file called __process.yaml__ that
contains instructions including which datasets to process. An example of the process.yaml file can be found in 
the root of this directory. The _processor_ will create a directory for each dataset that contains the input 
to the [_doc_ server](https://github.com/voxxlr/doc). When starting the server, the DATA_DIR must be passed as a parameter. 


