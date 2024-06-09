# Overview
The Voxxlr _processor_ converts input files into a format required by the Voxxlr _doc_ server. Currently supported are point clouds, drone maps in geotiff, CAD models in IFC and 360 images.

## Runing locally in vscode

Change the /data path in the "mounts" section of the .devcontainer.json to a path on your local machine. Create a process.yaml file including datasets in the /data directory and then open the project in vscode. After building the source, run the following command in a vscode terminal inside the dev container. 

```
python3 process.py /data
```

The appropriate output directory will be created in /data.

## Building the docker image

After cloning this repos run the following commands to build the docker image.

```sh
docker compose build
```

## Running the processor

Change the /data path in the "volumes" section of the docker-compose.yaml to a path on your local machine. Create a process.yaml file including datasets in the /data directory and then run

```sh
docker compose up
```

The appropriate output directory will be created in /data.
