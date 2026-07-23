snodec_add_test(Mqtt311PacketValidationTest Mqtt311PacketValidationTest.cpp)
target_include_directories(Mqtt311PacketValidationTest PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(Mqtt311PacketValidationTest PRIVATE cxx_std_20)
target_link_libraries(Mqtt311PacketValidationTest PRIVATE snodec-test-support snodec::mqtt snodec::mqtt-packets snodec::mqtt-server snodec::mqtt-server-packets)
set_tests_properties(Mqtt311PacketValidationTest PROPERTIES LABELS "unit;mqtt;mqtt311;validation" TIMEOUT 10)

snodec_add_test(MqttLifecycleTest MqttLifecycleTest.cpp)
target_include_directories(MqttLifecycleTest PRIVATE ${PROJECT_SOURCE_DIR})
target_compile_features(MqttLifecycleTest PRIVATE cxx_std_20)
target_link_libraries(
    MqttLifecycleTest
    PRIVATE snodec-test-support snodec::mqtt-server snodec::logger
)
set_tests_properties(
    MqttLifecycleTest PROPERTIES LABELS "unit;mqtt;protocol;lifecycle;log"
                                TIMEOUT 10
)
