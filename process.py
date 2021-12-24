import json
import os
import subprocess
import sys
import math
import psutil
import shutil
import glob
import time
import yaml

from io import BytesIO
from pathlib import Path

#resource.setrlimit(resource.RLIMIT_NOFILE, (131072, 131072))

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

        if datatype in [".e57", ".pts", ".laz"]: #"cloud"

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

        elif datatype in [".tiff"]: #"map"

            color = source["files"][0]["name"] if len(source["files"]) > 0  else ""
            elevation = source["files"][1]["name"] if len(source["files"]) > 1  else ""
            runVoxxlr("map/tiler", { 
                        "cpus": psutil.cpu_count(),
                        "memory": psutil.virtual_memory().free,
                        "color": color,
                        "elevation": elevation,
                        })

        elif datatype in [".jpg", ".jpeg"]: #"panorama"
    
            runVoxxlr("panorama/cuber", { 
                        "cpus": psutil.cpu_count(),
                        "memory": psutil.virtual_memory().free,
                        "file": source["files"][0]["name"],
                        })

        elif datatype in [".ifc", ".gltf"]:#"model"

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

        os.chdir("..")




                        
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