import subprocess
import os
import time
import threading
import ctypes
import sys
import socket

uuid = os.getlogin() + "_" + socket.gethostname()

def run(alert, outFile, parsed):
    wevtutilFetch = ["wevtutil", "epl", alert, f"{outFile}.evtx"]
    wevtutilDelete = ["wevtutil", "cl", alert]
    readEvtx = ["readEvtx.exe", f"{outFile}.evtx"]
    parse = ["evtx2json.exe", f"{outFile}.json", f"{parsed}"]
    verify = ["python3", "validator.py", f"{parsed}"]

    subprocess.run(wevtutilFetch, check=True)
    subprocess.run(readEvtx, check=True)
    subprocess.run(wevtutilDelete, check=True)

    parse_log = f"{parsed}_parse.log"
    verify_log = f"{parsed}_verify.log"

    with open(parse_log, "a") as p_log:
        subprocess.run(parse, check=True, stdout=p_log, stderr=p_log)

    with open(verify_log, "a") as v_log:
        subprocess.run(verify, check=True, stdout=v_log, stderr=v_log)

def logging(path, timestamp):
    print(os.listdir(path))

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
dir = os.path.dirname(os.path.realpath(__file__))
outDir = os.path.join(dir, "logs", uuid)
os.makedirs(outDir, exist_ok=True)

threads = []
for alert in alerts:
    outFile = os.path.join(outDir, f"{ts}_{alert}").replace("\\", "/")
    parsedFile = os.path.join(outDir, "parsed", f"{ts}_{alert}.json").replace("\\", "/")
    
    t = threading.Thread(target=run, args=(alert, outFile, parsedFile))
    t.start()
    threads.append(t)

for t in threads:
    t.join()