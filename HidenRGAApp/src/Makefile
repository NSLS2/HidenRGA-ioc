TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# Build the IOC application

PROD_IOC = HidenRGA
# HidenRGA.dbd will be created and installed
DBD += HidenRGA.dbd

# HidenRGA.dbd will be made up from these files:
HidenRGA_DBD += base.dbd
HidenRGA_DBD += asyn.dbd
HidenRGA_DBD += calc.dbd
HidenRGA_DBD += stream.dbd
HidenRGA_DBD += drvAsynIPPort.dbd
HidenRGA_DBD += aSubRecord.dbd
HidenRGA_DBD += system.dbd
HidenRGA_DBD += devIocStats.dbd
HidenRGA_DBD += asSupport.dbd
HidenRGA_DBD += ether_ip.dbd
HidenRGA_DBD += measValue.dbd
HidenRGA_DBD += caPutLog.dbd
HidenRGA_DBD += reccaster.dbd

# HidenRGA_registerRecordDeviceDriver.cpp derives from HidenRGA.dbd
HidenRGA_SRCS += HidenRGA_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
HidenRGA_SRCS_DEFAULT += HidenRGAMain.cpp
HidenRGA_SRCS_vxWorks += -nil-

HidenRGA_SRCS += measValue.c

# Add support from base/src/vxWorks if needed
#HidenRGA_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Link in the code from the support library
HidenRGA_LIBS += asyn calc stream
HidenRGA_LIBS += devIocStats autosave ether_ip caPutLog reccaster

# NOTE: To build SNL programs, SNCSEQ must be defined
# in the <top>/configure/RELEASE file

ifneq ($(SNCSEQ),)
    # Build sncExample into HidenRGASupport
    HidenRGA_SNCFLAGS += +d
    HidenRGA_DBD += sncHidenRGA.dbd
    HidenRGA_SRCS += sncDegas.stt
    HidenRGA_LIBS += seq pv

    # The following builds sncProgram as a standalone application
    PROD_HOST += sncProgram
    sncProgram_SNCFLAGS += +m
    sncProgram_SRCS += sncProgram.st
    sncProgram_LIBS += seq pv
    sncProgram_LIBS += $(EPICS_BASE_HOST_LIBS)
endif

# Finally link to the EPICS Base libraries
HidenRGA_LIBS += $(EPICS_BASE_IOC_LIBS)

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

