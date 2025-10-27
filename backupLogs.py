import subprocess
import os
import time
import threading
import ctypes
import sys
import socket
import zipfile

import config

uuid = os.getlogin() + "_" + socket.gethostname()
basePath = os.path.dirname(os.path.realpath(__file__))

pjoin = os.path.join

ts = time.strftime("%Y-%m-%d_%H-%M-%S")

def clearDirectory():
    files = os.listdir(config.rawDirectory)

    for file in files:
        p = pjoin(config.rawDirectory, file)
        os.remove(p)

def createZip(files, timestamp):
    print("Creating zip file ...")
    zipPath = f"{config.zipDirectory}/{timestamp}_{config.uuid}.zip"
    with zipfile.ZipFile(zipPath, "w", compression=zipfile.ZIP_DEFLATED) as zipf:
        for file in files:
            abs_path = pjoin(config.parsedDirectory, file)
            zipf.write(abs_path, arcname=file)
            os.remove(abs_path)
    print("Zip file created!")

def executeEVTX(alert, outFile, parsed):
    wevtutilFetch = ["wevtutil", "epl", alert, f"{outFile}.evtx"]
    wevtutilDelete = ["wevtutil", "cl", alert]
    readEvtx = ["native/readEvtx.exe", f"{outFile}.evtx"]
    parse = ["native/evtx2json.exe", f"{outFile}.json", f"{parsed}"]
    verify = ["python3", "src/validator.py", f"{parsed}"]

    subprocess.run(wevtutilFetch, check=True)
    subprocess.run(readEvtx, check=True)
    subprocess.run(wevtutilDelete, check=True)
    subprocess.run(parse, check=True)
    subprocess.run(verify, check=True)

def startUploadIfOK(timestamp):
    files = os.listdir(config.parsedDirectory)
    size = 0
    for file in files:
        size += os.path.getsize(pjoin(config.parsedDirectory, file))
    
    clearDirectory()

    if size >= config.chunkSize:
        print("Uploading ...") 
        createZip(files, timestamp)
    else:
        print("File size:", size / 1024 / 1024, "mb", "\nNo file upload")

def is_admin():
    try:
        return ctypes.windll.shell32.IsUserAnAdmin()
    except:
        return False

def main():
    if not os.path.isfile("native/evtx2json.exe") or not os.path.isfile("native/evtx2json.exe"):
        sys.exit("Run make first")

    if not is_admin():
        print("Run as administrator")
        sys.exit()

    os.makedirs(config.logDirectory, exist_ok=True)
    os.makedirs(config.rawDirectory, exist_ok=True)
    os.makedirs(config.parsedDirectory, exist_ok=True)
    os.makedirs(config.zipDirectory, exist_ok=True)

    threads = []
    for alert in config.logTypes:
        rawFilePath = pjoin(config.rawDirectory, f"{ts}_{alert}").replace("\\", "/")
        parsedFilePath = pjoin(config.parsedDirectory, f"{ts}_{alert}.json").replace("\\", "/")
        
        t = threading.Thread(target=executeEVTX, args=(alert, rawFilePath, parsedFilePath))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    startUploadIfOK(ts)

if __name__ == "__main__":
    main()