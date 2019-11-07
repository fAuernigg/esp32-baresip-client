#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#
 
#COMPONENT_ADD_INCLUDEDIRS := ../components/arduino/ ../components/pubsubclient/ ../components/re/include ../components/rem/include ../components/baresip/include/
#CFLAGS := -DUSE_VIDEO=1
ifdef MQTTSERVER
ifdef MQTTPORT
CXXFLAGS += -DMQTTSERVER=\"$(MQTTSERVER)\" -DMQTTPORT=$(MQTTPORT)
endif
endif
ifdef MQTTUSER
ifdef MQTTPASS
CXXFLAGS += -DMQTTUSER=\"$(MQTTUSER)\" -DMQTTPASS=\"$(MQTTPASS)\"
endif
endif
ifdef NOTLS
CXXFLAGS += -DNOTLS=1
endif
ifdef BUILDNR
CXXFLAGS += -DBUILDNR=\"$(BUILDNR)\"
endif

#COMPONENT_OBJEXCLUDE = \
#					   sipphone.o
