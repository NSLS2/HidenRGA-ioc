TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
DB += hiden_rga_global.template
DB += hiden_rga_bar.template
DB += hiden_rga_prfl.template
DB += hiden_rga_mid.template
DB += hiden_rga_ld.template
DB += hiden_rga_degas.template

DB_INSTALLS += $(EPICS_BASE)/db/iocAdminSoft.db
DB_INSTALLS += $(EPICS_BASE)/db/save_restoreStatus.db
DB_INSTALLS += $(EPICS_BASE)/db/reccaster.db

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

