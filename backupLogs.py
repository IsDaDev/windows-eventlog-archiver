import subprocess
import os
import time
import threading
import ctypes
import sys
import socket
import zipfile

uuid = os.getlogin() + "_" + socket.gethostname()
basePath = os.path.dirname(os.path.realpath(__file__))

pjoin = os.path.join

logDirectory = pjoin(basePath, "logs", uuid)
rawDirectory = pjoin(logDirectory, "raw")
parsedDirectory = pjoin(logDirectory, "parsed")
zipDirectory = pjoin(basePath, "zips")

alerts = ["Security", "Application", "System"]

ts = time.strftime("%Y-%m-%d_%H-%M-%S")

os.makedirs(logDirectory, exist_ok=True)
os.makedirs(rawDirectory, exist_ok=True)
os.makedirs(parsedDirectory, exist_ok=True)
os.makedirs(zipDirectory, exist_ok=True)

def clearDirectory():
    files = os.listdir(rawDirectory)

    for file in files:
        p = pjoin(rawDirectory, file)
        os.remove(p)

def createZip(files, timestamp):
    print("Creating zip file ...")
    zipPath = f"{zipDirectory}/{timestamp}_{uuid}.zip"
    with zipfile.ZipFile(zipPath, "w", compression=zipfile.ZIP_DEFLATED) as zipf:
        for file in files:
            abs_path = pjoin(parsedDirectory, file)
            zipf.write(abs_path, arcname=file)
            os.remove(abs_path)
    print("Zip file created!")

def executeEVTX(alert, outFile, parsed):
    wevtutilFetch = ["wevtutil", "epl", alert, f"{outFile}.evtx"]
    wevtutilDelete = ["wevtutil", "cl", alert]
    readEvtx = ["readEvtx.exe", f"{outFile}.evtx"]
    parse = ["evtx2json.exe", f"{outFile}.json", f"{parsed}"]
    verify = ["python3", "validator.py", f"{parsed}"]

    subprocess.run(wevtutilFetch, check=True)
    subprocess.run(readEvtx, check=True)
    subprocess.run(wevtutilDelete, check=True)
    subprocess.run(parse, check=True)
    subprocess.run(verify, check=True)

def startUploadIfOK(timestamp):
    BLOBSIZE = 100 * 1024 * 1024 # 100mb

    files = os.listdir(parsedDirectory)
    size = 0
    for file in files:
        size += os.path.getsize(pjoin(parsedDirectory, file))
    
    clearDirectory()

    if size >= BLOBSIZE:
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
    if not is_admin():
        print("Run as administrator")
        sys.exit()

    threads = []
    for alert in alerts:
        rawFilePath = pjoin(rawDirectory, f"{ts}_{alert}").replace("\\", "/")
        parsedFilePath = pjoin(parsedDirectory, f"{ts}_{alert}.json").replace("\\", "/")
        
        t = threading.Thread(target=executeEVTX, args=(alert, rawFilePath, parsedFilePath))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    startUploadIfOK(ts)

if __name__ == "__main__":
    main()