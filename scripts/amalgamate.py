import os
import shutil
import tarfile
import zipfile
import tempfile
import re
from pathlib import Path

# Configuration
SOURCE_DIR = Path("include/RPL")
OUTPUT_FILE = SOURCE_DIR / "RPL.hpp"

DEPS = {
    "frozen": {
        "url": "https://github.com/serge-sans-paille/frozen/archive/refs/tags/1.2.0.tar.gz",
        "type": "tar.gz",
        "include_src": "frozen-1.2.0/include",
    },
    "expected": {
        "url": "https://github.com/TartanLlama/expected/archive/refs/tags/v1.3.1.zip",
        "type": "zip",
        "include_src": "expected-1.3.1/include",
    },
    "cppcrc": {
        "url": "https://codeload.github.com/DarrenLevine/cppcrc/zip/refs/heads/main",
        "type": "zip",
        "include_src": "cppcrc-main",
    }
}

import subprocess

def download_extract(url, type):
    tmp_dir = tempfile.mkdtemp()
    file_name = url.split('/')[-1]
    file_path = os.path.join(tmp_dir, file_name)
    
    print(f"Downloading {url} using curl...")
    try:
        subprocess.run(["curl", "-L", url, "-o", file_path], check=True)
    except subprocess.CalledProcessError as e:
        print(f"Failed to download {url}: {e}")
        # Try wget as fallback
        print(f"Trying wget...")
        subprocess.run(["wget", url, "-O", file_path], check=True)
    
    extract_dir = os.path.join(tmp_dir, "extracted")
    os.makedirs(extract_dir, exist_ok=True)
    
    if type == "tar.gz":
        with tarfile.open(file_path, "r:gz") as tar:
            tar.extractall(path=extract_dir)
    elif type == "zip":
        with zipfile.ZipFile(file_path, 'r') as zip_ref:
            zip_ref.extractall(extract_dir)
            
    return extract_dir

class Amalgamator:
    def __init__(self, search_paths):
        self.search_paths = [Path(p) for p in search_paths]
        self.included_files = set()
        self.output_lines = []
        self.system_includes = set()
        
    def find_file(self, include_path, current_file_dir=None):
        # 1. Try relative to current file
        if current_file_dir:
            full_path = current_file_dir / include_path
            if full_path.exists() and full_path.is_file():
                return full_path.resolve()
        
        # 2. Try search paths
        for base in self.search_paths:
            full_path = base / include_path
            if full_path.exists() and full_path.is_file():
                return full_path.resolve()
        return None

    def process_file(self, file_path):
        file_path = Path(file_path).resolve()
        if file_path in self.included_files:
            return
        
        self.included_files.add(file_path)
        print(f"Processing {file_path}...")
        
        current_file_dir = file_path.parent
        
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
            
        # Robust guard removal: only remove the first #ifndef/#define and the LAST #endif
        # if they seem to be a file-level guard.
        first_ifndef_idx = -1
        first_define_idx = -1
        last_endif_idx = -1
        
        for i, line in enumerate(lines):
            clean_line = line.strip()
            if clean_line.startswith("#ifndef ") and first_ifndef_idx == -1:
                # Basic heuristic: guard often contains file name or "HPP"
                if "HPP" in clean_line or "H_" in clean_line or "FROZEN" in clean_line or "TL_" in clean_line:
                    first_ifndef_idx = i
            elif clean_line.startswith("#define ") and first_ifndef_idx != -1 and first_define_idx == -1:
                first_define_idx = i
            elif clean_line.startswith("#endif"):
                last_endif_idx = i
        
        # Only consider it a guard if ifndef and define are at the very beginning (ignoring comments/whitespace)
        is_guarded = False
        if first_ifndef_idx != -1 and first_define_idx != -1:
            # Check if there's significant code before the ifndef
            significant_code_before = False
            for i in range(first_ifndef_idx):
                if lines[i].strip() and not lines[i].strip().startswith(("/", "*")):
                    significant_code_before = True
                    break
            if not significant_code_before:
                is_guarded = True

        for i, line in enumerate(lines):
            clean_line = line.strip()
            
            if clean_line.startswith("#pragma once"):
                continue
                
            if is_guarded:
                if i == first_ifndef_idx or i == first_define_idx or i == last_endif_idx:
                    continue

            # Detect includes
            match = re.match(r'#include\s+(["<])([^">]+)([">])', line.strip())
            if match:
                quote_type = match.group(1)
                include_path = match.group(2)
                
                # System includes
                if quote_type == '<' and not self.is_local_include(include_path):
                    self.system_includes.add(line.strip())
                    continue
                
                # Local or dependency includes
                found_path = self.find_file(include_path, current_file_dir)
                if found_path:
                    self.process_file(found_path)
                else:
                    # Try with RPL/ prefix if it's missing
                    found_path = self.find_file("RPL/" + include_path, current_file_dir)
                    if found_path:
                        self.process_file(found_path)
                    else:
                        if quote_type == '<':
                            self.system_includes.add(line.strip())
                        else:
                            print(f"Warning: Could not find include file: {include_path}")
                            self.output_lines.append(line)
            else:
                self.output_lines.append(line)

    def is_local_include(self, include_path):
        # RPL and dependencies
        if include_path.startswith("RPL/") or \
           include_path.startswith("frozen/") or \
           include_path.startswith("tl/") or \
           include_path == "cppcrc.h":
            return True
        # Also check relative paths in the same directory
        return False

def main():
    # Setup dependencies
    tmp_deps_dir = Path(tempfile.mkdtemp())
    search_paths = [Path("include"), tmp_deps_dir]
    
    for name, config in DEPS.items():
        extract_path = download_extract(config["url"], config["type"])
        src_path = Path(extract_path) / config["include_src"]
        # Copy to a flatter structure in tmp_deps_dir
        if src_path.is_dir():
            for item in src_path.iterdir():
                dest = tmp_deps_dir / item.name
                if item.is_dir():
                    if dest.exists(): shutil.rmtree(dest)
                    shutil.copytree(item, dest)
                else:
                    shutil.copy2(item, dest)
        else:
            shutil.copy2(src_path, tmp_deps_dir / src_path.name)

    amalgamator = Amalgamator(search_paths)
    
    # Start from core files
    core_files = [
        "RPL/Utils/Def.hpp",
        "RPL/Deserializer.hpp",
        "RPL/Serializer.hpp",
        "RPL/Parser.hpp"
    ]
    
    for f in core_files:
        path = amalgamator.find_file(f)
        if path:
            amalgamator.process_file(path)
            
    # Assemble final file
    with open(OUTPUT_FILE, 'w', encoding='utf-8') as f:
        f.write("/**\n")
        f.write(" * RPL (RoboMaster Packet Library) - Single Header Version\n")
        f.write(" * Generated automatically. Do not edit directly.\n")
        f.write(" */\n\n")
        f.write("#ifndef RPL_SINGLE_HEADER_HPP\n")
        f.write("#define RPL_SINGLE_HEADER_HPP\n\n")
        
        # System includes
        for inc in sorted(list(amalgamator.system_includes)):
            f.write(f"{inc}\n")
        f.write("\n")
        
        # Content
        # Remove multiple empty lines
        content = "".join(amalgamator.output_lines)
        content = re.sub(r'\n\s*\n\s*\n', '\n\n', content)
        f.write(content)
        
        f.write("\n#endif // RPL_SINGLE_HEADER_HPP\n")

    print(f"Successfully generated {OUTPUT_FILE}")
    # Cleanup
    shutil.rmtree(tmp_deps_dir)

if __name__ == "__main__":
    main()
