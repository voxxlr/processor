# Overview
The Voxxlr processor converts input files into a format required by the voxxlr doc server. The processor is deployed
using docker and thus requied that the docker engine is already installed. 

## Building the docker image

After cloning this repos run the following commands to build the docker image.

```sh
cd processor
./docker-build.sh
```

Once done a new docker image called vx-processor should be listed when running 

```sh
docker images
```

## Running the processor

Run the following shell script 

```sh
./docker-run.sh DATA_DIR
```

to process datasets located in the DATA_DIR directory. The DATA_DIR must contain a file call process.yaml that
contains instructions for the processor including which datasets to process. An example of the process.yaml file can be found in 
the root of this directory. 

The processor will create a directory for dataset which contains the input to the doc server. When starting the server, the DATA_DIR must
be passed as a parameter. 


