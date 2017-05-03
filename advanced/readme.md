# Advanced

This sample will show you how to initiate firmware update by Azure Remote Monitoring solution and download a per-compiled executable file for demo purpose.

## Folder introduction

### 1.0

The sample code is the beginning for firmware update scenario.


### 2.0

The folder contains the new files for updating process, it contains two files:
	
- remote_monitoring
	
	The pre-compiled binary of remote_monitoring.c (2.0 version), it is an executable file.

- firmwarereboot.sh
	
	This file will execute reboot operation as soon as the "reomte_monitoring" downloads and replaces the old one.

### config

This folder contains a configuration file and a log file.

- deviceinfo

	This file should be updated with the real device's id and connection string.

- lastupdate

	A log file is responsible for recording firmware update steps.
	