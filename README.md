# Windows Event Log Archiver

A Python + C++ tool to automatically dump, parse, and archive Windows EVTX event logs. They are being saved as a zip file and can later be uploaded to an Azure Blob Storage for long-time retention (if configured).

The parser used is a modified version of this one: [xml2jsonParser](https://github.com/IsDaDev/xml2jsonParser)

## Layout

```
.
│
├── backupLogs.py               # Main script – exports, parses, validates, and archives EVTX logs
├── config.py                   # Configuration file
├── validator.py                # Validates JSON output for correctness
├── Makefile                    # Build script for the native binaries
├── README.md
├── .gitignore
├── requirements.txt
│
├── native/                     # Compiled binaries and source code
│   ├── evtx2json.cpp           # XML->JSON parser
│   ├── readEvtx.cpp            # Convert EVTX to XML
│   └── headers/
│       └── json.hpp            # Important Header for readEvtx.cpp
│
├── logs/                       # Working directory for log exports
│   └── <username>_<hostname>/
│       ├── raw/                # Raw EVTX files
│       └── parsed/             # Converted JSON log files
│
└── zips/                       # Final archived backups (zipped JSON logs)
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

### Configuration-file

You can configure multiple settings in the configuration file `config.py` like compression, filepaths and if you want to upload the logs to a Azure Blob Storage you also have to configure the credentials for that storage account.

### If you want to upload toa storage account

1. Create Storage account
2. Create Container
3. Change configurations in `config.py`

### Prerequesites

- g++ (MinGW or MSVC)
- make
- Python3

### Commands

```shell
cd windows-eventlog-archiver
pip install -r requirements.txt
make
```

### Execute

```
python3 backupLogs.py
```
