# This makefile triggers the targets in the application.mk

# The default value "../../.." assumes that this makefile is placed in the 
# folder xdk110/Apps/<App Folder> where the BCDS_BASE_DIR is the parent of 
# the xdk110 folder.
BCDS_BASE_DIR ?= ../../..

# Macro to define Start-up method. change this macro to "CUSTOM_STARTUP" to have custom start-up.
export BCDS_SYSTEM_STARTUP_METHOD = DEFAULT_STARTUP
export BCDS_APP_NAME = BoschXDKCloudConnectivity
export BCDS_APP_DIR = $(CURDIR)
export BCDS_APP_SOURCE_DIR = $(BCDS_APP_DIR)/source

#Please refer BCDS_CFLAGS_COMMON variable in application.mk file
#and if any addition flags required then add that flags only in the below macro 
# export BCDS_CFLAGS_COMMON = -DXDK_SERIAL_TRACE

#List all the application header file under variable BCDS_XDK_INCLUDES in a similar pattern as below
export BCDS_XDK_INCLUDES = \
		-I$(BCDS_APP_SOURCE_DIR)/fota \
		-I$(BCDS_APP_SOURCE_DIR)/protected \
		-I$(BCDS_APP_SOURCE_DIR)
	
#List all the application source file under variable BCDS_XDK_APP_SOURCE_FILES in a similar pattern as below
export BCDS_XDK_APP_SOURCE_FILES = \
	$(BCDS_APP_SOURCE_DIR)/Main.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mClient.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mInterface.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObjects.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mUtil.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Device.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Humidity.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Barometer.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Gyroscope.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Temperature.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Magnetometer.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Illuminance.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_Accelerometer.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_LightControl.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_SensorDevice.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_ConnectivityMonitoring.c \
	$(BCDS_APP_SOURCE_DIR)/Lwm2mObjects/Lwm2mObject_AlertNotification.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceAccelerometer.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceEnvironment.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceGyroscope.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceMagnetometer.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceIlluminance.c \
	$(BCDS_APP_SOURCE_DIR)/SensorDeviceUtil.c \
	$(BCDS_APP_SOURCE_DIR)/CfgParser.c \
	$(BCDS_APP_SOURCE_DIR)/ButtonsMan.c \
	$(BCDS_APP_SOURCE_DIR)/SntpTime.c \
	$(BCDS_APP_SOURCE_DIR)/DnsUtil.c \
	$(BCDS_APP_SOURCE_DIR)/FotaDownload.c \
	$(BCDS_APP_SOURCE_DIR)/FaultHandler.c \

.PHONY: clean	debug release flash_debug_bin flash_release_bin

clean: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk clean

debug: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk debug
	
release: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk release
	
clean_Libraries:
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk clean_libraries
	
flash_debug_bin: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk flash_debug_bin
	
flash_release_bin: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk flash_release_bin
	
cleanlint: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk cleanlint
	
lint: 
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk lint
	
cdt:
	$(MAKE) -C $(BCDS_BASE_DIR)/xdk110/Common -f application.mk cdt	
