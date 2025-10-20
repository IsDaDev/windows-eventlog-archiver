import subprocess
import os
from os import path
import time
import threading
import ctypes
import sys
import socket

uuid = os.getlogin() + "_" + socket.gethostname()
MAX_SIZE = 100 * 1024 * 1024 # 100mb

def split_json_lines(input_file, output_folder):
    os.makedirs(output_folder, exist_ok=True)
    chunk = []
    chunk_index = 1
    current_size = 0

    filename = input_file.replace('.json', '')

    with open(input_file, "r", encoding="utf-8") as f:
        for line in f:
            line_size = len(line.encode("utf-8"))
            if current_size + line_size > MAX_SIZE:
                output_file = os.path.join(output_folder, f"${filename}_{chunk_index}.json")
                with open(output_file, "w", encoding="utf-8") as out:
                    out.writelines(chunk)
                chunk_index += 1
                chunk = []
                current_size = 0
            chunk.append(line)
            current_size += line_size

    if chunk:
        output_file = os.path.join(output_folder, f"${filename}_{chunk_index}.json")
        with open(output_file, "w", encoding="utf-8") as out:
            out.writelines(chunk)

def run(alert, outFile):
    wevtutilFetch = ["wevtutil", "epl", alert, outFile]
    wevtutilDelete = ["wevtutil", "cl", alert]
    convertEvtx = ["readEvtx.exe", outFile]
    subprocess.run(wevtutilFetch, check=True)
    subprocess.run(convertEvtx, check=True)
    subprocess.run(wevtutilDelete, check=True)

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

if not is_admin():
    ctypes.windll.shell32.ShellExecuteW(
        None, "runas", sys.executable, " ".join(sys.argv), None, 1
    )
    sys.exit()


alerts = ["Security", "Application", "System"]

ts = time.strftime("%Y-%m-%d_%H-%M-%S")
dir = path.dirname(path.realpath(__file__))
outDir = path.join(dir, "logs", uuid)
os.makedirs(outDir, exist_ok=True)

threads = []
for alert in alerts:
    outFile = path.join(outDir, f"{ts}_{alert}.evtx")
    t = threading.Thread(target=run, args=(alert, outFile))
    t.start()
    threads.append(t)

for t in threads:
    t.join()