cmake_minimum_required(VERSION 3.0)

<<<<<<< HEAD
add_library(net SHARED ClientSocket.cpp ConnectedSocket.cpp InetAddress.cpp ServerSocket.cpp Socket.cpp SocketMultiplexer.cpp TimerManager.cpp SocketReadManager.cpp SocketWriteManager.cpp SocketExceptionManager.cpp Request.cpp Response.cpp AcceptedSocket.cpp Descriptor.cpp Timer.cpp FileReader.cpp)
=======
add_library(net SHARED
        Multiplexer.cpp
        Descriptor.cpp
        Socket.cpp
        SocketReader.cpp
        SocketWriter.cpp
        ServerSocket.cpp
        ConnectedSocket.cpp
        ClientSocket.cpp
        TimerManager.cpp
        ReadManager.cpp
        WriteManager.cpp
        ExceptionManager.cpp
        InetAddress.cpp
        Timer.cpp
        FileReader.cpp
        StartUp.cpp
        Cookie.cpp
        )
>>>>>>> MatthiasKoettritsch

target_link_libraries(net LINK_PUBLIC magic)

target_include_directories(net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#find_program(iwyu_path NAMES include-what-you-use iwyu)
#if(NOT iwyu_path)
#  message(FATAL_ERROR "Could not find the program include-what-you-use")
#endif()
#set_property(TARGET net PROPERTY CXX_INCLUDE_WHAT_YOU_USE ${iwyu_path})
