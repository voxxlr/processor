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
def runVoxxlr(name,args):
   if os.name == "posix":
        name = f"{processor}/bin/{name}"
        sys.stdout.write(name+" \""+json.dumps(args).replace('"','\\"')+"\"\n")
        process = subprocess.Popen(name+" '"+json.dumps(args)+"'", stdout=subprocess.PIPE, stderr=sys.stdout, shell=True,cwd=".")
   else:
        name = f"{processor}/bin/{name}.exe"
        sys.stdout.write(name+"\""+json.dumps(args).replace('"','\\"')+"\"\n")
        process = subprocess.Popen([name,json.dumps(args)], stdout=subprocess.PIPE, stderr=sys.stdout, shell=True)

   process.communicate()
   process.wait()

   response = {}
   with open("process.json", "r") as file:
        response = json.load(file)
   os.remove("process.json")
   return response

#load config file
os.chdir(sys.argv[1])

with open("process.yaml", "r") as file:

    for config in yaml.load_all(file, Loader=yaml.SafeLoader):

        #create output directory
        directory = Path(config["file"]).stem
        if not os.path.exists(directory):
            os.makedirs(directory)
        os.chdir(directory)

        datatype = Path(config["file"]).suffix

        if config['type'] == 'cloud' and datatype in [".e57", ".pts", ".laz"]: 

            response = runVoxxlr("cloud/importer", 
                {
                "file": [f'../{config["file"]}'],
                "filter": config["filter"] if "filter" in config else None,
                "radius": config["radius"] if "radius" in config else None,
                "coords": config["coords"] if "coords" in config else "right-z",
                "transform": config["transform"] if "transform" in config else None,
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
    
            os.remove(f'./{dataset}.ply')
    
            for file in glob.glob('./*.ply'):
                os.remove(file)
            for file in glob.glob('./*.log'):
                os.remove(file)

        elif config['type'] == 'map' and datatype in [".tiff"]:

            color = source["files"][0]["name"] if len(source["files"]) > 0  else ""
            elevation = source["files"][1]["name"] if len(source["files"]) > 1  else ""
            runVoxxlr("map/tiler", { 
                        "color": color,
                        "elevation": elevation,
                        })

        elif config['type'] == 'panorama' and datatype in [".jpg", ".jpeg"]: 
    
            runVoxxlr("panorama/cuber", { 
                        "file": source["files"][0]["name"],
                        })

        elif config['type'] == 'model' and datatype in [".ifc", ".gltf"]:

            if datatype == ".ifc":
         
                runVoxxlr("model/ifc", { 
                            "file": f'../{config["file"]}',
                            "scalar": config["scalar"] if "scalar" in config else 1
                            })
         
            elif datatype == ".gltf":
         
                runVoxxlr("model/gltf", { 
                            "file": f'../{config["file"]}',
                            "scalar": config["scalar"] if "scalar" in config else 1
                            })

        os.chdir("..")


