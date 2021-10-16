import json
import os
import subprocess
import sys
import math
import psutil
import shutil
import glob
import time
from io import BytesIO

#setup directory and logfile
startTime = time.time()

if not os.path.exists("process"):
    os.makedirs("process")
os.chdir("process")

if not os.path.exists("root"):
    os.makedirs("root")
#logF = open("/process.log",'w')
logF = sys.stdout

logF.write("################\n")
root = os.path.dirname(os.path.abspath(__file__))
logF.write(root+"\n")

def runVoxxlr(name,args):
    name = f"{root}/bin/{name}.exe"
    logF.write(name+"\""+json.dumps(args).replace('"','\\"')+"\"\n")
    #logF.write(json.dumps(args,sort_keys=True,indent=4)+"\n")
    process = subprocess.Popen([name,json.dumps(args)], stdout=logF, stderr=logF, shell=True)
    stdout, stderr = process.communicate()
    process.wait()

config = {
    "meta": 
    {
        "source":
        {
            "files": [
                { 
#                   "name": "d:/home/voxxlr/data/cloud/BAUM.e57"
#                   "name": "d:/home/voxxlr/data/cloud/navvis/E57_HQ3rdFloor_SLAM_5mm.e57",
#                    "name": "d:/home/voxxlr/data/cloud/distro-canyon.las"
                    "name": "d:/home/voxxlr/data/cloud/GORKA1.las"
#                   "name": "d:/home/voxxlr/data/ifc/Stairs.ifc"
                }
            ]
        }
    },
    "type": 1,
}

with open("meta.json", "w") as meta:
    meta.write(json.dumps(config["meta"])+"\n")

source = config["meta"]["source"];

if config["type"] == 1: #"cloud"

    runVoxxlr("cloud/importer", { 
                "file": source["files"][0]["name"],
                "coords": config["coords"] if "coords" in config else "right-z",
                "scalar": config["scalar"] if "scalar" in config else 1,
                "resolution": config["resolution"] if "resolution" in config else None
             })
    
    if not "resolution" in config:
        runVoxxlr("cloud/analyzer", { })
   
    runVoxxlr("cloud/filter", { "density": None })

    runVoxxlr("cloud/packetizer", { })

elif config["type"] == 2:#"map"

    color = source["files"][0]["name"] if len(source["files"]) > 0  else ""
    elevation = source["files"][1]["name"] if len(source["files"]) > 1  else ""
    runVoxxlr("map/tiler", { 
                "cpus": psutil.cpu_count(),
                "memory": psutil.virtual_memory().free,
                "color": color,
                "elevation": elevation,
                })

elif config["type"] == 3:#"panorama"
    
    runVoxxlr("panorama/cuber", { 
                "cpus": psutil.cpu_count(),
                "memory": psutil.virtual_memory().free,
                "file": source["files"][0]["name"],
                })

elif config["type"] == 4:#"model"

    for file in source["files"]:

        if file["name"].endswith("gltf"):
         
            runVoxxlr("model/gltf", { 
                        "cpus": psutil.cpu_count(),
                        "memory": psutil.virtual_memory().free,
                        "file": file["name"],
                        "scalar": config["scalar"] if "scalar" in config else 1
                        })
         
        elif file["name"].endswith("ifc") or file["name"].endswith("IFC"):
         
            runVoxxlr("model/ifc", { 
                        "cpus": psutil.cpu_count(),
                        "memory": psutil.virtual_memory().free,
                        "file": file["name"],
                        "scalar": config["scalar"] if "scalar" in config else 1
                        })

