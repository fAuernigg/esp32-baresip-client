
# include $(IDF_PATH)/../esp32phone/components/baresip/mk/mod.mk $(IDF_PATH)/../esp32phone/components/baresip/mk/modules.mk $(IDF_PATH)/../esp32phone/components/baresip/src/srcs.mk

COMPONENT_ADD_INCLUDEDIRS +=  ../pubsubclient/src
SRCS	+=	$(IDF_PATH)/../esp32phone/components/pubsubclient/src/*.cpp
