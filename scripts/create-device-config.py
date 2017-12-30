#!/usr/bin/env python3.5
#==============================================================================
#description     :Creates all configuration files for register the device to
#                 the cloud 
#author          :denis@dschoener.de
#python_version  :3.5
#==============================================================================

import argparse
from pathlib import Path
import os
import sys
import subprocess
import re

default_dir = '.'
default_key_file = 'rsa_key.pem'
default_cert_file = 'rsa_cert.pem'
default_config_file = 'conf5.json'

def eprint(*_args, **kwargs):
    print(*_args, file=sys.stderr, **kwargs)
    sys.exit()

def parse_args():
    global default_dir
    parser = argparse.ArgumentParser(description='Create new device device')
    parser.add_argument('--config-dir', default=default_dir, help='device configuration directory (def: ' + default_dir + ')')
    parser.add_argument('--device-id', required=True, help='device identifier. Must start with alphabetic characters (i.e. spark-00eefe01)')
    return parser.parse_args()

def create_certs(device_id):
    cmd = ["openssl", "req", "-x509", "-newkey", "rsa:2048",
           "-keyout", default_key_file, "-nodes",
           "-out", default_cert_file, "-subj", "/CN=" + device_id]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    return (process.returncode == 0)
    
def create_device_config(device_id):
    global default_config_file
    with open(default_config_file, 'w') as f:
        f.write('{\n'
                '  "device": {\n'
                '    "id": \"' + device_id + '\"\n'
                '  },\n'
                '  "mqtt": {\n'
                '    "client_id": "' + device_id + '",\n' 
                '    "ssl_cert": "' + default_cert_file + '",\n'
                '    "ssl_key": "' + default_key_file + '",\n' 
                '    "ssl_ca_cert": ""\n'
                '  }\n'
                '}')
    
def create_files(path, device_id):
    dir = Path(path)
    if not re.match("spark-[0-9a-f]*", device_id):
        eprint("device id does not match constraints")
        
    print("device id: '" + device_id + "'")
    if not dir.exists():
        dir.mkdir()
    if dir.exists():
        os.chdir(path)
        create_certs(device_id)
        create_device_config(device_id)
    else:
        eprint("failed to create dir '" + path + "'")
        
def main():
    args = parse_args()
    create_files(args.config_dir, args.device_id)
    
if __name__ == "__main__":
    main()