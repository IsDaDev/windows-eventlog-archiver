from os import path, getlogin
from socket import gethostname

# Unique Identifier if multiple logs are safed at the same destination
uuid = getlogin() + "_" + gethostname()

# array containing all logs to collect
logTypes = ["Security", "Application", "System"]

# max filesize before they get zipped in mb
chunkSize = 100 * 1024 * 1024 # 100mb

# how far archives should be compressed
# level 1: 200mb -> 10-15mb
# level 9: 200mb -> 6-8mb
compressionsLevel = 9

# toggle if 0mb files should be included
includeEmptyFiles = False

# ======================================================================
# file upload to a Microsoft Azure Blob Storage
# ======================================================================

# file upload to Microsoft Azure Blob Storage
uploadToAzure = False

# full storage account url 
accountUrl = "https://example.blob.core.windows.net/"

# storage container
container = "example"

# toggle if zip files should be kept locally after uploading
retainZipLocally = True

# ======================================================================
# paths are based on the file location of the executable file by default
# since its a windows tool, only windows paths are allowed, e.g. C:/...
# ======================================================================

# base path, default where the executable lies
basePath = path.abspath(path.dirname(__file__))

# location where the logs are safed
logDirectory = path.join(basePath, "logs", uuid)

# location of raw logs and the parsed logs, default location is inside the logdirectory and UUID
rawDirectory = path.join(logDirectory, "raw")
parsedDirectory = path.join(logDirectory, "parsed")

# location for the finished zip files
zipDirectory = path.join(basePath, "zips")