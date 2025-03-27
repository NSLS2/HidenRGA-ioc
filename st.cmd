#!bin/linux-x86/HidenRGA

## Location of stream protocol files
epicsEnvSet("ENGINEER",                 "hxu x4394")
epicsEnvSet("LOCATION",                 "XF21IDA")
epicsEnvSet("STREAM_PROTOCOL_PATH",     "protocol")

epicsEnvSet("SYS",   			"XF:03-VA")
epicsEnvSet("SYSPORT",  		"XF03VA")
epicsEnvSet("CTSYS",			"XF:03-VA")
epicsEnvSet("TSADR",			"192.168.1.50")

epicsEnvSet("EPICS_CA_AUTO_ADDR_LIST", 	"NO")
epicsEnvSet("EPICS_CA_ADDR_LIST",	"192.168.1.255")

## Register all support components
dbLoadDatabase "dbd/HidenRGA.dbd"
HidenRGA_registerRecordDeviceDriver pdbbase

drvAsynIPPortConfigure("$(SYSPORT)_RGAC1","$(TSADR):4014")

## Load record instances
dbLoadRecords "db/asynRecord.db"
dbLoadRecords "db/xf-rga1.db"

## Set this to see messages from mySub
#var rga_subDebug 1

## Run this to trace the stages of iocInit
#traceIocInit

iocInit

## Start any sequence programs
#seq sncExample, "user=hxuHost"
