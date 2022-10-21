cmake_minimum_required(VERSION 3.5)

set(IOT_MQTT_SERVER_BROKER_CPP Broker.cpp Message.cpp Session.cpp SubscribtionTree.cpp RetainTree.cpp)
set(IOT_MQTT_SERVER_BROKER_H Broker.h Message.h Session.h SubscribtionTree.h RetainTree.h)

add_library(
    mqtt-broker STATIC ${IOT_MQTT_SERVER_BROKER_CPP}
                       ${IOT_MQTT_SERVER_BROKER_H}
)
add_library(snodec::mqtt-broker ALIAS mqtt-broker)

set_property(TARGET mqtt-broker PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    mqtt-broker PUBLIC "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
)
