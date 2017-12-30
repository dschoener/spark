#!/usr/bin/env python3.5
#==============================================================================
#description     :Registers a device to the Google device registry 
#author          :denis@dschoener.de
#python_version  :3.5
#==============================================================================

import argparse
from pathlib import Path
import os
import sys
import subprocess
import json

project_config = { 
    'project_id': None,
    'registry_id': None,
    'device_region': None,
    'device_id': None,
    'device_cert': None
    }

def eprint(*_args, **kwargs):
    print(*_args, file=sys.stderr, **kwargs)
    sys.exit()

def parse_args():
    global default_dir
    parser = argparse.ArgumentParser(description='Register a new device to the Google device registry')
    parser.add_argument('-p', '--project-config', required=True, help='project configuration file (JSON)')
    parser.add_argument('-d', '--device-config', required=True, help='device configuration file (JSON)')
    return parser.parse_args()

def register_device():
    cmd = ["gcloud", "beta", "iot", "devices", "create", project_config['device_id'],
           "--project=" + project_config['project_id'],
           "--region=" + project_config['device_region'],
           "--registry=" + project_config['registry_id'],
           "--public-key=path=" + project_config['cert_file'] + ",type=rsa-x509-pem"]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    return (process.returncode == 0)
    
def parse_project_config(file):
    with open(file) as fd:
        cont = json.load(fd)
        if type(cont) is dict:
            if 'gcp' in cont:
                gcp = cont['gcp']
                if not 'project' in gcp:
                    eprint("missing project id")
                project_config['project_id'] = str(gcp['project'])
                if not 'registry' in gcp:
                    eprint("missing registry id")
                project_config['registry_id'] = str(gcp['registry'])
                if not 'region' in gcp:
                    eprint("missing device region")
                project_config['device_region'] = str(gcp['region'])
    return True

def parse_device_config(file):
    with open(file) as fd:
        cont = json.load(fd)
        if type(cont) is dict:
            if 'mqtt' in cont:
                mqtt = cont['mqtt']
                if not 'client_id' in mqtt:
                    eprint("missing client id")
                project_config['device_id'] = str(mqtt['client_id'])
                if not 'ssl_cert' in mqtt:
                    eprint("missing SSL certificate")
                project_config['cert_file'] = create_relative_path(os.path.dirname(file), str(mqtt['ssl_cert']))
    return True

def create_relative_path(origin, path):
    _path = Path(os.path.join(origin, path))
    return str(_path)
    
def main():
    args = parse_args()
    parse_project_config(args.project_config)
    parse_device_config(args.device_config)
    register_device()
    
if __name__ == "__main__":
    main()