import json
import pycurl
import os
import subprocess
import sys
import math
import psutil
import shutil
import glob
import time
import resource
from io import BytesIO
from google.cloud import datastore
from PIL import Image
import googleapiclient.discovery

#change file limits
resource.setrlimit(resource.RLIMIT_NOFILE, (131072, 131072))

startTime = time.time()

# get machine metadata
data = BytesIO()
crl = pycurl.Curl() 
crl.setopt(crl.URL, 'http://metadata/computeMetadata/v1/instance/attributes/config')
crl.setopt(pycurl.HTTPHEADER, ['Metadata-Flavor: Google'])
crl.setopt(crl.WRITEFUNCTION,data.write)
crl.perform() 
config = json.loads(data.getvalue().decode('UTF-8'));
id = str(config["id"])

root = os.path.dirname(os.path.abspath(__file__))

# setup processing directory
if not os.path.exists("process"):
    os.makedirs("process")
    subprocess.run(['/usr/bin/gsutil','-q','-m','cp','-r','gs://voxxlr-upload/'+id+'/*','./process'])
os.chdir("process")
with open("meta.json", "w") as meta:
    meta.write(json.dumps(config["meta"])+"\n")

#logF = sys.stdout
logF = open("process.log",'w')
logF.write("################\n")
logF.write(json.dumps(config,sort_keys=True,indent=4)+"\n")
logF.write("gs://voxxlr-upload/"+id+"\n")

def runVoxxlr(name,args):
    name = f"{root}/bin/{name}"
    logF.write(name+" \""+json.dumps(args).replace('"','\\"')+"\"\n")
    #logF.write(json.dumps(args,sort_keys=True,indent=4)+"\n")
    #process = subprocess.Popen([name,json.dumps(args)], stdout=logF, stderr=logF, shell=True)
    process = subprocess.Popen(name+" '"+json.dumps(args)+"'", stdout=logF, stderr=logF, shell=True,cwd=".")
    stdout, stderr = process.communicate()
    process.wait()

# pocess dataset
with open("meta.json", "w") as meta:
    meta.write(json.dumps(config["meta"])+"\n")

if not os.path.exists("root"):
    os.makedirs("root")

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

    runVoxxlr("cloud/filter", {  "density": config["density"] if "density" in config else None })
 
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


logF.write("\n################ BUCKET\n")
client = datastore.Client()
#get account from datastore
account = client.get(client.key("account", config["account"]))

#copy processed data to bucket
logF.write("copying cloud to bucket\n")
process = subprocess.Popen('gsutil -m -h "Cache-Control:public max-age=3600" -q cp -r -Z ./root/** gs://voxxlr-'+str(account["created"])+'/'+id+'/', stdout=logF, stderr=logF, shell=True) 
stdout, stderr = process.communicate()
process.wait()

logF.write("\n################ DATASTORE\n")
#create cloud table entry
document = datastore.Entity(key=client.key("cloud", -config["id"]), exclude_from_indexes=(['root']))
document["tags"] = config["tags"]
document["type"] = config["type"]
document["account"] = config["account"]
document["bucket"] = account["created"]
document["root"] = open('root.json', 'r').read()

logF.write("create document datastore entry\n")
client.put(document)
        
#create metadata table entry
meta = datastore.Entity(key=client.key("meta", -config["id"]), exclude_from_indexes=(['content']))
meta["content"] = open('meta.json', 'r').read()
logF.write("create meta datastore entry\n")
client.put(meta)

#update meter
meter = client.get(client.key("meter", config["account"]))
processing = json.loads(meter["processing"])
processing.remove(int(id))
meter["processing"] = json.dumps(processing)
meter["compute.minutes"] = meter["compute.minutes"] + round((time.time() - startTime)/60)
meter["timestamp"] = round(startTime*1000)
logF.write("updating meter\n")
client.put(meter)


#notify user
if "notify" in config:
    message = """{"from":{"email":"info@voxxlr.com"},"subject":"Upload to Voxxlr","content":[{"type":"text/plain","value":"Hello, your recent upload to Voxxlr has completed processing, and the data set is now accessible in your account. Please let us know about any problems by replying to this email... The Voxxlr Devs"}],"personalizations":[{"to":[{"email":"EMAIL"}]}]}""".replace("EMAIL", config["account"])
    crl = pycurl.Curl()
    crl.setopt(crl.URL, 'https://api.sendgrid.com/v3/mail/send')
    crl.setopt(pycurl.POST, 1)
    crl.setopt(pycurl.HTTPHEADER, [
        f"Authorization: Bearer {config['notify']}",
        'Content-Type: application/json',
        'Content-Length: ' + str(len(message))
        ])
    crl.setopt(pycurl.POSTFIELDS, message)
    logF.write("sending email\n")
    crl.perform()
    crl.close()

if "shutdown" in config:

    # shutdown instance 
    data = BytesIO()
    crl = pycurl.Curl()
    crl.setopt(crl.URL, 'http://metadata/computeMetadata/v1/project/project-id')
    crl.setopt(pycurl.HTTPHEADER, ['Metadata-Flavor: Google'])
    crl.setopt(crl.WRITEFUNCTION,data.write)
    crl.perform()
    project = data.getvalue().decode("UTF=8")

    data = BytesIO()
    crl = pycurl.Curl()
    crl.setopt(crl.URL, 'http://metadata/computeMetadata/v1/instance/name')
    crl.setopt(pycurl.HTTPHEADER, ['Metadata-Flavor: Google'])
    crl.setopt(crl.WRITEFUNCTION,data.write)
    crl.perform()
    instance = data.getvalue().decode("UTF=8")

    data = BytesIO()
    crl = pycurl.Curl()
    crl.setopt(crl.URL, 'http://metadata/computeMetadata/v1/instance/zone')
    crl.setopt(pycurl.HTTPHEADER, ['Metadata-Flavor: Google'])
    crl.setopt(crl.WRITEFUNCTION,data.write)
    crl.perform()
    path = data.getvalue().decode("UTF=8")
    array = path.split("/")
    zone = array[len(array)-1]

    compute = googleapiclient.discovery.build('compute', 'v1')
    request = compute.instances().delete(project=project, zone=zone, instance=instance)
    response = request.execute()
