from os import path
import os
import socket

# Unique Identifier if multiple logs are safed at the same destination
uuid = os.getlogin() + "_" + socket.gethostname()

# array containing all logs to collect
logTypes = ["Security", "Application", "System"]

# max filesize before they get zipped in mb
chunkSize = 100 * 1024 * 1024 # 100mb


# ======================================================================
# paths are based on the file location of the executable file by default
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