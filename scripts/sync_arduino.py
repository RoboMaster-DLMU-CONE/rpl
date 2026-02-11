import os
import shutil
import urllib.request
import tarfile
import zipfile
import tempfile
from pathlib import Path

# Configuration
SOURCE_DIR = Path("include/RPL")
DEST_DIR = Path("rpl-arduino/src/RPL")
DEPS_DIR = Path("rpl-arduino/src/3rdparty")
ROOT_SRC = Path("rpl-arduino/src")
DEPS = {
    "frozen": {
        "url": "https://github.com/serge-sans-paille/frozen/archive/refs/tags/1.2.0.tar.gz",
        "type": "tar.gz",
        "include_src": "frozen-1.2.0/include/frozen",
        "include_dest": "frozen"
    },
    "expected": {
        "url": "https://github.com/TartanLlama/expected/archive/refs/tags/v1.3.1.zip",
        "type": "zip",
        "include_src": "expected-1.3.1/include/tl",
        "include_dest": "tl"
    },
    "cppcrc": {
        "url": "https://codeload.github.com/DarrenLevine/cppcrc/zip/refs/heads/main",
        "type": "zip",
        "include_src": "cppcrc-main/cppcrc.h",
        "include_dest": "cppcrc.h", 
        "is_file": True
    }
}

def clean_dir(path):
    if path.exists():
        shutil.rmtree(path)
    path.mkdir(parents=True, exist_ok=True)

def download_extract(url, type, is_file=False):
    tmp_dir = tempfile.mkdtemp()
    file_name = url.split('/')[-1]
    file_path = os.path.join(tmp_dir, file_name)
    
    print(f"Downloading {url}...")
    urllib.request.urlretrieve(url, file_path)
    
    extract_dir = os.path.join(tmp_dir, "extracted")
    os.makedirs(extract_dir, exist_ok=True)
    
    if type == "tar.gz":
        with tarfile.open(file_path, "r:gz") as tar:
            tar.extractall(path=extract_dir)
    elif type == "zip":
        with zipfile.ZipFile(file_path, 'r') as zip_ref:
            zip_ref.extractall(extract_dir)
            
    return extract_dir

def sync_rpl():
    print("Syncing RPL core...")
    if DEST_DIR.exists():
        shutil.rmtree(DEST_DIR)
    shutil.copytree(SOURCE_DIR, DEST_DIR)

def sync_deps():
    clean_dir(DEPS_DIR)
    
    for name, config in DEPS.items():
        print(f"Processing {name}...")
        extract_path = download_extract(config["url"], config["type"])
        
        src = Path(extract_path) / config["include_src"]
        dest = DEPS_DIR / config["include_dest"]
        
        if config.get("is_file"):
            shutil.copy2(src, dest)
        else:
            shutil.copytree(src, dest)
            
        print(f"Installed {name} to {dest}")

def create_entry_header():
    # Create the main header that Arduino users will include
    with open("rpl-arduino/src/RPL.h", "w") as f:
        f.write("#pragma once\n")
        f.write("#include \"RPL/Parser.hpp\"\n")
        f.write("#include \"RPL/Serializer.hpp\"\n")
        f.write("#include \"RPL/Deserializer.hpp\"\n")

def main():
    # Create target directory structure
    clean_dir(ROOT_SRC)
    
    sync_rpl()
    sync_deps()
    create_entry_header()
    
    print("Synchronization complete!")

if __name__ == "__main__":
    main()
