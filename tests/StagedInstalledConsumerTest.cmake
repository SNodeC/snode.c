set(stage "${SNODEC_BUILD_DIR}/staged-installed-consumer")
set(prefix "${stage}/prefix")
set(consumer "${stage}/consumer.cpp")
set(codex_consumer_source "${CMAKE_CURRENT_LIST_DIR}/installed/codex")
set(codex_consumer_build "${stage}/codex-consumer-build")
file(REMOVE_RECURSE "${stage}")
file(MAKE_DIRECTORY "${stage}")
execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${SNODEC_BUILD_DIR}" --prefix
            "${prefix}" RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "staged install failed")
endif()
foreach(
    private_header IN
    ITEMS core/EventLoop.h
          core/EventMultiplexer.h
          core/DescriptorEventPublisher.h
          core/TimerEventPublisher.h
          ai/openai/codex/detail/CodexErrorInfoCodec.h
          ai/openai/codex/detail/ClientOperationCodec.h
          ai/openai/codex/detail/ConversationCodec.h
          ai/openai/codex/detail/ClientOperationCodecDescriptors.inc
          ai/openai/codex/detail/ConversationUnionCodecDescriptors.inc
          ai/openai/codex/detail/ThreadItemCodecDescriptors.inc
          ai/openai/codex/detail/ResponseItemCodecDescriptors.inc
          ai/openai/codex/detail/ServerNotificationCodecDescriptors.inc
          ai/openai/codex/detail/DecodeDiagnostic.h
          ai/openai/codex/detail/ProtocolCodec.h
          ai/openai/codex/detail/EventDecoder.h
          ai/openai/codex/detail/ItemDecoder.h
          ai/openai/codex/detail/ProtocolSurfaceRegistry.h
          ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc
          ai/openai/codex/detail/ServerRequestDecoder.h
          ai/openai/codex/detail/ThreadCodec.h
          ai/openai/codex/detail/Transport.h
          ai/openai/codex/detail/TurnCodec.h
          ai/openai/codex/stdio/StdioTransport.h
          ai/openai/codex/backend/detail/BackendCoreImpl.h
          ai/openai/codex/backend/detail/PreserveUnmodeledTypedEvent.h
          ai/openai/codex/frontend/detail/CodecImpl.h
)
    if(EXISTS "${prefix}/include/snode.c/${private_header}")
        message(FATAL_ERROR "private header installed: ${private_header}")
    endif()
endforeach()

set(
    expected_codex_public_headers
    "AppServerClient.h"
    "Protocol.h"
    "backend/BackendCommand.h"
    "backend/BackendCore.h"
    "backend/BackendEvent.h"
    "backend/BackendState.h"
    "backend/FrontendSession.h"
    "backend/Reducer.h"
    "backend/Snapshot.h"
    "frontend/BackendAdapter.h"
    "frontend/Codec.h"
    "frontend/EventCoalescer.h"
    "frontend/EventJournal.h"
    "frontend/Messages.h"
    "frontend/Protocol.h"
    "frontend/UpdateBatch.h"
    "stdio/Client.h"
    "typed/Accounts.h"
    "typed/Client.h"
    "typed/CodexErrorInfo.h"
    "typed/Conversation.h"
    "typed/Events.h"
    "typed/Items.h"
    "typed/Results.h"
    "typed/ServerRequests.h"
    "typed/Threads.h"
    "typed/Turns.h"
    "typed/Types.h"
)
list(SORT expected_codex_public_headers)
file(
    GLOB_RECURSE installed_codex_public_headers
    LIST_DIRECTORIES FALSE
    RELATIVE "${prefix}/include/snode.c/ai/openai/codex"
    "${prefix}/include/snode.c/ai/openai/codex/*.h"
)
list(SORT installed_codex_public_headers)
if(NOT "${installed_codex_public_headers}" STREQUAL
   "${expected_codex_public_headers}")
    message(
        FATAL_ERROR
            "installed Codex public-header inventory changed\nexpected: ${expected_codex_public_headers}\nobserved: ${installed_codex_public_headers}"
    )
endif()

foreach(
    codex_library IN
    ITEMS snodec-ai-openai-codex snodec-ai-openai-codex-backend
          snodec-ai-openai-codex-frontend
)
    if(NOT EXISTS "${prefix}/lib/lib${codex_library}.so.1")
        message(
            FATAL_ERROR
                "Codex A1 rebuild boundary must retain SOVERSION 1: missing lib${codex_library}.so.1"
        )
    endif()
endforeach()

file(
    GLOB_RECURSE installed_entries
    LIST_DIRECTORIES TRUE
    "${prefix}/*"
)
foreach(installed_entry IN LISTS installed_entries)
    string(REPLACE "\\" "/" installed_entry "${installed_entry}")
    get_filename_component(installed_name "${installed_entry}" NAME)
    if(installed_entry MATCHES "/tools/codex/"
       OR installed_entry MATCHES "/tests/component/codex/fixtures/"
       OR installed_entry MATCHES "/app-server-schema/"
       OR installed_entry MATCHES "/app-server-surface/"
       OR installed_entry MATCHES "/app-server-evidence/"
       OR installed_entry MATCHES "/app-server-fixtures/"
       OR installed_entry MATCHES "/app-server-protocol-source/"
       OR installed_name STREQUAL "app_server_a1_1.py"
       OR installed_name STREQUAL "app_server_a1_1_closure.py"
       OR installed_name STREQUAL "app_server_surface.py"
       OR installed_name STREQUAL "app_server_contracts.py"
       OR installed_name STREQUAL "app_server_fixtures.py"
       OR installed_name STREQUAL "draft07.py"
       OR installed_name STREQUAL "CodexA11AuditToolTest.py"
       OR installed_name STREQUAL "PROVENANCE.json"
       OR installed_name STREQUAL "LICENSE.openai-codex"
       OR installed_name STREQUAL "NOTICE.openai-codex"
       OR installed_name STREQUAL "ProtocolSurfaceRegistry.cpp"
       OR installed_name STREQUAL "ProtocolSurfaceRegistry.h"
       OR installed_name STREQUAL "ProtocolSurfaceRegistryData.inc"
       OR installed_name STREQUAL "ConversationCodec.cpp"
       OR installed_name STREQUAL "ConversationCodec.h"
       OR installed_name STREQUAL "ClientOperationCodec.cpp"
       OR installed_name STREQUAL "ClientOperationCodec.h"
       OR installed_name STREQUAL "ClientOperationCodecDescriptors.inc"
       OR installed_name STREQUAL "ConversationUnionCodecDescriptors.inc"
       OR installed_name STREQUAL "ThreadItemCodecDescriptors.inc"
       OR installed_name STREQUAL "ResponseItemCodecDescriptors.inc"
       OR installed_name STREQUAL "ServerNotificationCodecDescriptors.inc"
       OR installed_name STREQUAL "operation-contracts.json"
       OR installed_name STREQUAL "module-slice-assignment.json"
       OR installed_name STREQUAL "nested-reachability.json"
       OR installed_name STREQUAL "schema-completeness-evidence.json"
       OR installed_name STREQUAL "fixture-coverage.json"
       OR installed_name STREQUAL "schema-keywords.json"
       OR installed_name STREQUAL "a1-1-start-state.json"
       OR installed_name STREQUAL "a1-1-implementation-plan.json"
       OR installed_name STREQUAL "a1-1-type-closure.json"
       OR installed_name STREQUAL "a1-1-operation-production-coverage.json"
       OR installed_name STREQUAL "a1-1-notification-production-coverage.json"
       OR installed_name STREQUAL "a1-1-closure-report.json"
       OR installed_name STREQUAL "CodexA11OperationResultCorpusTest.cpp"
       OR installed_name STREQUAL "CodexA11OperationAggregateValueCorpusTest.cpp"
       OR installed_name STREQUAL "CodexConversationB4NestedCodecTest.cpp"
       OR installed_name STREQUAL "CodexA11OperationWireTest.cpp"
       OR installed_name STREQUAL "a1-1-conversation-domain.md"
       OR installed_name STREQUAL "typescript-audit.json"
       OR installed_name MATCHES "^Protocol.*(Data|Evidence|Descriptors)\\.inc$"
       OR (installed_entry MATCHES "/ai/openai/codex/"
           AND installed_name MATCHES "\\.inc$")
    )
        message(FATAL_ERROR "private Codex evidence artifact installed: ${installed_entry}")
    endif()
endforeach()
file(
    WRITE "${consumer}"
    "#include <core/socket/stream/SocketServer.h>\n#include <core/socket/stream/SocketClient.h>\n#include <net/in/stream/legacy/SocketServer.h>\n#include <net/in/stream/legacy/SocketClient.h>\n#include <express/legacy/in/Server.h>\nint main() { return 0; }\n"
)
set(exe "${stage}/consumer")
execute_process(
    COMMAND
        "${CMAKE_CXX_COMPILER}" -std=c++20 "${consumer}"
        "-I${prefix}/include/snode.c" "-L${prefix}/lib"
        "-L${prefix}/lib/snode.c/web/http" "-Wl,-rpath,${prefix}/lib"
        "-Wl,-rpath,${prefix}/lib/snode.c/web/http" -lsnodec-core
        -lsnodec-core-socket -lsnodec-core-socket-stream -lsnodec-net
        -lsnodec-net-in -lsnodec-net-in-phy -lsnodec-net-in-phy-stream
        -lsnodec-net-in-stream -lsnodec-core-socket-stream-legacy
        -lsnodec-net-in-stream-legacy -lsnodec-http -lsnodec-http-server
        -lsnodec-http-server-express -lsnodec-http-server-express-legacy-in -o
        "${exe}"
    RESULT_VARIABLE compile_result
    OUTPUT_VARIABLE compile_output
    ERROR_VARIABLE compile_error
)
message(
    STATUS
        "Installed consumer compile command: ${CMAKE_CXX_COMPILER} -std=c++20 ${consumer} -I${prefix}/include/snode.c -L${prefix}/lib -L${prefix}/lib/snode.c/web/http -Wl,-rpath,${prefix}/lib -Wl,-rpath,${prefix}/lib/snode.c/web/http -lsnodec-core -lsnodec-core-socket -lsnodec-core-socket-stream -lsnodec-net -lsnodec-net-in -lsnodec-net-in-phy -lsnodec-net-in-phy-stream -lsnodec-net-in-stream -lsnodec-core-socket-stream-legacy -lsnodec-net-in-stream-legacy -lsnodec-http -lsnodec-http-server -lsnodec-http-server-express -lsnodec-http-server-express-legacy-in -o ${exe}"
)
if(NOT compile_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer compile failed\n${compile_output}\n${compile_error}"
    )
endif()
execute_process(
    COMMAND "${exe}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_output
    ERROR_VARIABLE run_error
)
if(NOT run_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed consumer execution failed\n${run_output}\n${run_error}"
    )
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}" -S "${codex_consumer_source}" -B
        "${codex_consumer_build}" "-DCMAKE_PREFIX_PATH=${prefix}"
        "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    RESULT_VARIABLE codex_configure_result
    OUTPUT_VARIABLE codex_configure_output
    ERROR_VARIABLE codex_configure_error
)
if(NOT codex_configure_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed Codex CMake consumer configure failed\n${codex_configure_output}\n${codex_configure_error}"
    )
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${codex_consumer_build}"
    RESULT_VARIABLE codex_build_result
    OUTPUT_VARIABLE codex_build_output
    ERROR_VARIABLE codex_build_error
)
if(NOT codex_build_result EQUAL 0)
    message(
        FATAL_ERROR
            "installed Codex CMake consumer build failed\n${codex_build_output}\n${codex_build_error}"
    )
endif()
