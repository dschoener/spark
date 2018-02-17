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

default_project_config = "project-config.json"
default_expire_days = 365 * 100

command_require_map = {
    'create-certs': ['device_id', 'ssl_key_file', 'ssl_cert_file'],
    'create-device-config': ['device_id', 'device_conf_file'],
    'register-device': ['device_id', 'device_region', 'registry_id', 'ssl_cert_file']
    }
command_map = {
    'create-certs': 'create_certs',
    'create-device-config': 'create_device_config',
    'register-device': 'register_device'
    }

project_config = { 
    'project_id': None,
    'registry_id': None,
    'device_region': None,
    'device_id': None,
    'ssl_cert_file': None
    }

def eprint(*_args, **kwargs):
    print(*_args, file=sys.stderr, **kwargs)
    sys.exit()

def check_required_parameters(command):
    for param in command_require_map[command]:
        value = project_config[param]
        if value is None or len(value) == 0:
            eprint("missing or empty parameter (" + param + ") value")
    
def ckeck_if_device_registered():
    cmd = ["gcloud", "beta", "iot", "devices", "list", 
           "--project=" + project_config['project_id'],
           "--region=" + project_config['device_region'],
           "--registry=" + project_config['registry_id']]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    exists = False
    if project_config['device_id'] in str(out):
        exists = True
    return exists

def register_device():
    update_or_create = "create"
    if ckeck_if_device_registered():
        print("device '" + project_config['device_id'] + ' exists')
        update_or_create = "update"
        
    cmd = ["gcloud", "beta", "iot", "devices", update_or_create, project_config['device_id'],
           "--project=" + project_config['project_id'],
           "--region=" + project_config['device_region'],
           "--registry=" + project_config['registry_id'],
           "--public-key=path=" + project_config['ssl_cert_file'] + ",type=rsa-x509-pem"]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    return (process.returncode == 0)

def create_certs():
    global default_expire_days
    cmd = ["openssl", "req", "-x509", "-newkey", "rsa:2048",
           "-keyout", project_config['ssl_key_file'], "-nodes",
           "-out", project_config['ssl_cert_file'],
           "-subj", "/CN=" + project_config['device_id'],
           "-days", str(default_expire_days)]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    print("device SSL key file created: " + project_config['ssl_key_file'])
    print("device SSL key file created: " + project_config['ssl_cert_file'])

def create_device_config():
    with open(project_config['device_conf_file'], 'w') as f:
        f.write('{\n'
                '  "device": {\n'
                '    "id": \"' + project_config['device_id'] + '\"\n'
                '  },\n'
                '  "mqtt": {\n'
                '    "client_id": "' + project_config['device_id'] + '",\n' 
                '    "ssl_cert": "' + os.path.basename(project_config['ssl_cert_file']) + '",\n'
                '    "ssl_key": "' + os.path.basename(project_config['ssl_key_file']) + '",\n' 
                '    "ssl_ca_cert": "' + os.path.basename(project_config['ssl_ca_cert_file']) + '"\n'
                '  },\n'
                '  "gcp": {\n'
                '    "project": "' + project_config['project_id'] + '",\n'
                '    "registry": "' + project_config['registry_id'] + '",\n'
                '    "region": "' + project_config['device_region'] + '",\n'
                '    "device": "' + project_config['device_id'] + '",\n'
                '    "key": "' + os.path.basename(project_config['ssl_key_file']) + '"\n'
                '  }\n'
                '}')
    print("device configuration created: " + project_config['device_conf_file'])
        
def parse_project_config(file):
    ''' Parses the project configuration file '''
    with open(file) as fd:
        cont = json.load(fd)
        if 'project' in cont:
            cont = cont['project']
            if 'gcp' in cont:
                gcp = cont['gcp']
                if 'project' in gcp:
                    project_config['project_id'] = str(gcp['project'])
                if 'registry' in gcp:
                    project_config['registry_id'] = str(gcp['registry'])
                if 'region' in gcp:
                    project_config['device_region'] = str(gcp['region'])
                if 'mqtt-server' in gcp:
                    project_config['mqtt_server'] = str(gcp['mqtt-server'])
            if 'device' in cont:
                device = cont['device']
                if 'id' in device:
                    project_config['device_id'] = str(device['id'])
            if 'files' in cont:
                files = cont['files']
                if 'ssl_cert_file' in files:
                    project_config['ssl_cert_file'] = str(files['ssl_cert_file'])
                if 'ssl_key_file' in files:
                    project_config['ssl_key_file'] = str(files['ssl_key_file'])
                if 'ssl_ca_cert_file' in files:
                    project_config['ssl_ca_cert_file'] = str(files['ssl_ca_cert_file'])
                if 'device_conf_file' in files:
                    project_config['device_conf_file'] = str(files['device_conf_file'])            
    return True

def create_relative_path(origin, path):
    _path = Path(os.path.join(origin, path))
    return str(_path)

def run_command(command):
    if command in command_map.keys():
        check_required_parameters(command)
        eval(command_map[command])()
    else:
        eprint("unknown command: " + command)
    
def parse_args():
    global default_project_config
    parser = argparse.ArgumentParser(description='Register a new device to the Google device registry')
    parser.add_argument('-p', '--project-config', required=False,
                         help='project configuration file (def: ' + default_project_config + ")")
    command_list = ', '.join(command_map.keys())
    parser.add_argument('-c', '--command', required=True,
                         help='command to be executed (commands: ' + command_list + ')')
    return parser.parse_args()

def main():
    global default_project_config
    args = parse_args()
    project_config = args.project_config if args.project_config != None else default_project_config
    parse_project_config(project_config)
    run_command(args.command)
    
if __name__ == "__main__":
    main()