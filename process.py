import json
import os
import subprocess
import sys
import math
import shutil
import glob
import yaml

from io import BytesIO
from pathlib import Path

if os.name == "posix":
    import resource
    softLimit, hardLimit = resource.getrlimit(resource.RLIMIT_NOFILE)
    resource.setrlimit(resource.RLIMIT_NOFILE, (hardLimit, hardLimit))

processor = os.path.dirname(os.path.abspath(__file__))
def runTask(name,args,res=True):

    if os.name == "posix":
        name = f"{processor}/bin/{name}"
        sys.stdout.write(name+" \""+json.dumps(args).replace('"','\\"')+"\"\n")
        process = subprocess.Popen(name+" '"+json.dumps(args)+"'", stdout=subprocess.PIPE, stderr=sys.stdout, shell=True,cwd=".")
    else:
        name = f"{processor}/bin/{name}.exe"
        sys.stdout.write(name+"\""+json.dumps(args).replace('"','\\"')+"\"\n")
        process = subprocess.Popen([name,json.dumps(args)], stdout=subprocess.PIPE, stderr=sys.stdout, shell=True)

    response, error = process.communicate()
    process.wait()
    if res:
        return json.loads(response.decode('utf-8'))
    else:
        return {}


def processCloud(input, debug, process):

    files = []
    for file in input["file"]:
        files.append(f'../{file}')

    response = runTask("cloud/importer", 
    {
        "file": files,
        "coords": input["coords"] if "coords" in input else "right-z",
        "transform": input["transform"] if "transform" in input else None
    })
    
    dataset = response["file"]

    #apply average filter
    if 'resolution' not in process or process['resolution'] == 'auto':
        resolution = runTask("cloud/analyzer", { "file": f'./{dataset}' })["resolution"]
    else:
        resolution = process["resolution"]
    
    if "filter" in process:
        runTask("cloud/filter",
            { 
                "filter": process["filter"],
                "resolution": resolution,
                "file": f'./{dataset}'
            })

    runTask("cloud/packetizer", {  "file": f'./{dataset}' })
    
    os.remove(f'./{dataset}.ply')

    if not "ply" in debug:
        for file in glob.glob('./*.ply'):
            os.remove(file)


#load config file
os.chdir(sys.argv[1])
with open("process.yaml", "r") as file:

    for config in yaml.load_all(file, Loader=yaml.SafeLoader):

        output = config["output"]

        directory = str(output["directory"])
        if not os.path.exists(directory):
            os.makedirs(directory)
        os.chdir(directory)

        input = config["input"]

        debug = config["debug"] if "debug" in config else []

        if config['type'] == 'cloud': 

            processCloud(input, debug, config["process"])

        elif config['type'] == 'map':

            runTask("map/tiler", { 
                        "color": f'../{input["color"]}',
                        "elevation": f'../{input["elevation"]}',
                        }, False)

        elif config['type'] == 'panorama': 
    
            runTask("panorama/cuber", { 
                        "file": f'../{input["file"]}',
                        })

        elif config['type'] == 'model':

            if input["file"].endswith(".ifc"):
         
                runTask("model/ifc", { 
                            "file": f'../{input["file"]}',
                            "scalar": config["scalar"] if "scalar" in config else 1
                            })
         
            elif input["file"].endswith(".gltf"):
         
                runTask("model/gltf", { 
                            "file": f'../{input["file"]}',
                            "scalar": config["scalar"] if "scalar" in config else 1
                            })


        if not "log" in debug:
            for file in glob.glob('./*.log'):
                os.remove(file)

        os.chdir("..")


