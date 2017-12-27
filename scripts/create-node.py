###

import argparse
from pathlib import Path
import os
import sys
import subprocess
import uuid

default_dir = 'fs'
default_key_file = 'rsa_key.pem'
default_cert_file = 'rsa_cert.pem'
default_config_file = 'conf5.json'

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)
    sys.exit()
    
def parse_args():
    global default_dir
    parser = argparse.ArgumentParser(description='Create new device node')
    parser.add_argument('--dest', default=default_dir, help='destination directory (def: ' + default_dir + ')')
    parser.add_argument('--node-id', help='node identifier 48 bit hex number (i.e. MAC)')
    return parser.parse_args()

def create_certs(node_uuid):
    cmd = ["openssl", "req", "-x509", "-newkey", "rsa:2048",
           "-keyout", default_key_file, "-nodes",
           "-out", default_cert_file, "-subj", "/CN=" + node_uuid]
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE)
    out, err = process.communicate()
    if process.returncode != 0:
        eprint(err)
    return (process.returncode == 0)
    
def create_device_config(node_uuid):
    global default_config_file
    with open(default_config_file, 'w') as f:
        f.write('{\n'
                '  "device": {\n'
                '    "id": \"' + node_uuid + '\"\n'
                '  },\n'
                '  "mqtt": {\n'
                '    "client_id": "' + node_uuid + '",\n' 
                '    "ssl_cert": "' + default_cert_file + '",\n'
                '    "ssl_key": "' + default_key_file + '",\n' 
                '    "ssl_ca_cert": ""\n'
                '  }\n' 
                '}')
    
def create_files(path, node_id):
    dir = Path(path)
    node_uuid = None
    if node_id is not None:
        num = int(node_id, 16)
        node_uuid = str(uuid.uuid1(node=num))
    else:
        node_uuid = str(uuid.uuid1())
    print("device id: '" + node_uuid + "'")
    if not dir.exists():
        dir.mkdir()
    if dir.exists():
        os.chdir(path)
        create_certs(node_uuid)
        create_device_config(node_uuid)
    else:
        eprint("failed to create dir '" + path + "'")
        
def main():
    args = parse_args()
    create_files(args.dest, args.node_id)
    
if __name__ == "__main__":
    main()