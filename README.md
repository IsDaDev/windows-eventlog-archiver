# Windows Event Log Archiver

A Python + C++ tool to automatically dump, parse, and archive Windows EVTX event logs. They are being saved as a zip file and can later be uploaded to an Azure Blob Storage for long-time retention (work in progress).

## Layout

```
windows-eventlog-archiver/
│
├── backupLogs.py               # Main script – exports, parses, validates, and archives EVTX logs
├── validator.py                # Validates JSON output for correctness
├── readEvtx.cpp                # Reads Windows Event Logs (.evtx) and dumps as plaintext
├── evtx2json.cpp               # Converts raw EVTX data to structured JSON
│
├── headers/                    # Containing header files
│   └── nlohmann/json.hpp       # Headerfile for jsondump
│
├── logs/                       # Working directory for log exports
│   └── <user>_<hostname>/      # Automatically created per machine
│       ├── raw/                # Temporary .evtx files
│       └── parsed/             # Converted .json log files
│
├── zips/                       # Final archived backups (zipped JSON logs)
│
└── README.md                   # Documentation
```

### Directory explanations

- **logs/** → Temporary working folder; cleared after zipping/uploading
- **zips/** → Contains final archived .zip files (named `<timestamp>_<user>_<hostname>.zip`)

## Function

1. Dumps all EVTX-logs specified in `backupLogs.py` in the `alerts`-array into logs/<username>\_<hostname>. By default this includes Application, System, Security.
2. Logs are then read by `readEvtx.exe` and dumped as a readable format.
3. Logs are converted into valid JSON by `evtx2json.exe` and validated by `validator.py`.
4. Checks the size of the aggregated logs
   - If the size of all aggregated files is less than 100mb it skips
   - If the size is greater than 100mb, the files are zipped

## How to install and run

### Prerequesites

- g++ (MinGW or MSVC)
- Python3

### Commands

```shell
cd windows-eventlog-archiver
g++ readEvtx.cpp -o readEvtx -lwevtapi
g++ evtx2json.cpp -o evtx2json
```

### Execute

```
python3 backupLogs.py
```
