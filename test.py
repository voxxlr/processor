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
from pathlib import Path

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

'''
test me
if os.name == "posix":
    ulimit -Sn 4096
    process = subprocess.Popen("ulimit -Sn 4096", stdout=logF, stderr=logF, shell=True,cwd=".")
    stdout, stderr = process.communicate()
    process.wait()
'''

def runVoxxlr(name,args):
   if os.name == "posix":
        name = f"sudo {root}/bin/{name}"
        logF.write(name+" \""+json.dumps(args).replace('"','\\"')+"\"\n")
        #logF.write(json.dumps(args,sort_keys=True,indent=4)+"\n")
        #process = subprocess.Popen([name,json.dumps(args)], stdout=logF, stderr=logF, shell=True)
        process = subprocess.Popen(name+" '"+json.dumps(args)+"'", stdout=subprocess.PIPE, stderr=logF, shell=True,cwd=".")
   else:
        name = f"{root}/bin/{name}.exe"
        logF.write(name+"\""+json.dumps(args).replace('"','\\"')+"\"\n")
        #logF.write(json.dumps(args,sort_keys=True,indent=4)+"\n")
        process = subprocess.Popen([name,json.dumps(args)], stdout=subprocess.PIPE, stderr=logF, shell=True)

   response, error = process.communicate()
   process.wait()
   return json.loads(response.decode('utf-8'))


config = {
    "meta": 
    {
        "source":
        {
            "files": [
                { 
#                    "name": "d:/home/voxxlr/data/cloud/BAUM.e57"
#                     "name": "d:/home/voxxlr/data/cloud/1637810132834_759 LatestPC.e57"
#                    "name": "d:/home/voxxlr/data/cloud/1592244748825_Gill_Residence_1-0.e57"
#                  "name": "d:/home/voxxlr/data/cloud/navvis/E57_HQ3rdFloor_SLAM_5mm.e57",
#                    "name": "d:/home/voxxlr/data/cloud/distro-canyon.las"
#                    "name": "d:/home/voxxlr/data/cloud/GORKA1.las"
                   "name": "d:/home/voxxlr/data/ifc/Stairs.ifc"
                }
            ]
        }
    },
    "type": 4,
}

with open("meta.json", "w") as meta:
    meta.write(json.dumps(config["meta"])+"\n")

source = config["meta"]["source"];

if config["type"] == 1: #"cloud"

    file = source["files"][0]["name"]

    response = runVoxxlr("cloud/importer", 
        {
        "file": [file],
        #"radius": 10,
        "coords": config["coords"] if "coords" in config else "right-z",
        "scalar": config["scalar"] if "scalar" in config else 1,
        #"separate": True
        })

    dataset = response["files"][0]

    #apply average filter
    if not "resolution" in config:
        resolution = runVoxxlr("cloud/analyzer", { "file": f'./{dataset}' })["resolution"]
    else:
        resolution = config["resolution"]
 
    runVoxxlr("cloud/filter",
            { 
            "resolution": resolution,
            "file": f'./{dataset}'
            })

    runVoxxlr("cloud/packetizer", {  "file": f'./{dataset}' })

    '''
    if len(response["files"]) > 1:

        #in case of some e57 files. filter each separately and then combine
        summary = []
        for dataset in response["files"]:

            dir = Path(file).stem
            if not os.path.exists(dataset):
                os.makedirs(dataset)
            os.chdir(dataset)
    
            analysis = runVoxxlr("cloud/analyzer", { "file": f'../{dataset}' })
           
            runVoxxlr("cloud/filter",
                    { 
                    "density": config["density"] if "density" in config else None, 
                    "resolution": analysis["resolution"],
                    #"resolution": 0.0012,
                    "file": f'../{dataset}'
                    })

            summary.append(analysis)

            os.chdir("..")    

        #combine all scans into one
        dataset = [ f'{file}.ply' for file in response["files"] ]

        runVoxxlr("cloud/importer", 
                {
                "file": dataset,
                "output": "combined.ply",
                "coords": "right-y",
                })

        #apply average filter
        resolution = 0;
        for analysis in summary:
            resolution += analysis["resolution"]
        resolution /= len(summary)
 
        runVoxxlr("cloud/filter",
                { 
                "resolution": resolution,
                "file": "./combined"
                })

        if not os.path.exists("./combined"):
            os.makedirs("./combined")
        os.chdir("./combined")

        runVoxxlr("cloud/packetizer", {  "file": "../combined" })
    '''

  

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

