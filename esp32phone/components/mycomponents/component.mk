#ENABLE_ESP8266Audio := y
#ENABLE_ESP8266_Spiram := y
ENABLE_libre := y
ENABLE_rem := y
ENABLE_baresip := y
ENABLE_pubsubclient := y


ALLCOMPONENTS := $(patsubst $(COMPONENT_PATH)/../%,%,$(wildcard $(COMPONENT_PATH)/../*))

define MY_COMPONENT_ENABLED
$(if $(ENABLE_$(1)),y)
endef

MY_COMPONENTS := $(foreach COMP,$(sort $(ALLCOMPONENTS)),$(if $(call MY_COMPONENT_ENABLED,$(COMP)),$(COMP)))

$(info my components enabled in build: $(MY_COMPONENTS))

# Expand all subdirs under $(1)
define EXPAND_SUBDIRS
$(sort $(dir $(wildcard $(1)/* $(1)/*/* $(1)/*/*/* $(1)/*/*/*/* $(1)/*/*/*/*/*)))
endef

# Macro returns SRCDIRS for library
define MY_COMPONENT_GET_SRCDIRS
	$(if $(wildcard $(COMPONENT_PATH)/../$(1)/src/.), 			\
		$(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)/src), 			\
		$(filter-out $(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)/examples),  	\
			$(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)) 		\
		) 									\
	)
endef

define MY_COMPONENT_GET_INCDIRS
	$(if $(wildcard $(COMPONENT_PATH)/../$(1)/include/.), 			\
		$(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)/include), 			\
		$(filter-out $(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)/examples),  	\
			$(call EXPAND_SUBDIRS,$(COMPONENT_PATH)/../$(1)) 		\
		) 									\
	)
endef

# Make a list of all srcdirs in enabled libraries
MY_COMPONENTS_SRCDIRS := $(patsubst $(COMPONENT_PATH)/%,%,$(foreach COMP,$(MY_COMPONENTS),$(call MY_COMPONENT_GET_SRCDIRS,$(COMP))))
MY_COMPONENTS_INCDIRS := $(patsubst $(COMPONENT_PATH)/%,%,$(foreach COMP,$(MY_COMPONENTS),$(call MY_COMPONENT_GET_INCDIRS,$(COMP))))

#$(info Arduino libraries src dirs: $(MY_COMPONENTS_SRCDIRS))

#COMPONENT_ADD_INCLUDEDIRS :=  $(MY_COMPONENTS_SRCDIRS) $(MY_COMPONENTS_INCDIRS)
COMPONENT_ADD_INCLUDEDIRS := $(MY_COMPONENTS_INCDIRS) ../ESP8266Audio/src/ ../ESP8266_Spiram/src/
COMPONENT_PRIV_INCLUDEDIRS := $(MY_COMPONENTS_SRCDIRS) $(MY_COMPONENTS_INCDIRS)
COMPONENT_SRCDIRS := $(MY_COMPONENTS_SRCDIRS)

#CXXFLAGS +=
# XXX: common for C/C++
CXXFLAGS += -DHAVE_INTTYPES_H -DUSE_VIDEO=1
CPPFLAGS += -DHAVE_INTTYPES_H -DSHARE_PATH="\"../share\"" -DUSE_VIDEO=1
CFLAGS    += -DUSE_VIDEO=1
USE_OPENSSL_AES	:= yes

#$(info my src/inc folders: $(COMPONENT_SRCDIRS))
