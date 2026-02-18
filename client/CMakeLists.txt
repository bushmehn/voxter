cmake_minimum_required(VERSION 3.21)
project(VoxterClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.10 REQUIRED COMPONENTS
    Core
    Gui
    Quick
    QuickControls2
    Network
    Multimedia
    WebSockets
    Test
)

qt_standard_project_setup(REQUIRES 6.10)

if (COMMAND qt_policy)
    qt_policy(SET QTP0004 NEW)
endif()

set(LIVEKIT_SDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/livekit-sdk-0.3.1/livekit-sdk-windows-x64-0.3.1")
find_package(LiveKit CONFIG REQUIRED PATHS "${LIVEKIT_SDK_DIR}/lib/cmake/LiveKit" NO_DEFAULT_PATH)
set_target_properties(LiveKit::livekit PROPERTIES
    MAP_IMPORTED_CONFIG_DEBUG Release
    MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release
    MAP_IMPORTED_CONFIG_MINSIZEREL Release
)

add_library(livekit_ffi SHARED IMPORTED GLOBAL)
set_target_properties(livekit_ffi PROPERTIES
    IMPORTED_IMPLIB "${LIVEKIT_SDK_DIR}/lib/livekit_ffi.dll.lib"
    IMPORTED_LOCATION "${LIVEKIT_SDK_DIR}/bin/livekit_ffi.dll"
)

set(CLIENT_SOURCES
    src/main.cpp
    src/app/Application.cpp
    src/store/AppStore.cpp
    src/services/ApiClient.cpp
    src/services/GatewayClient.cpp
    src/services/SecureStorage.cpp
    src/services/NativeVoiceEngine.cpp
    src/viewmodels/AuthViewModel.cpp
    src/viewmodels/GuildViewModel.cpp
    src/viewmodels/ChatViewModel.cpp
    src/viewmodels/VoiceViewModel.cpp
    src/viewmodels/ProfileViewModel.cpp
)

set(CLIENT_HEADERS
    src/app/Application.h
    src/store/AppStore.h
    src/services/ApiClient.h
    src/services/GatewayClient.h
    src/services/SecureStorage.h
    src/services/NativeVoiceEngine.h
    src/viewmodels/AuthViewModel.h
    src/viewmodels/GuildViewModel.h
    src/viewmodels/ChatViewModel.h
    src/viewmodels/VoiceViewModel.h
    src/viewmodels/ProfileViewModel.h
)

set(CLIENT_QML
    qml/Main.qml
    qml/screens/LoginScreen.qml
    qml/screens/MainScreen.qml
    qml/components/ServerList.qml
    qml/components/ChannelList.qml
    qml/components/ChatView.qml
    qml/components/MessageComposer.qml
    qml/components/MembersPanel.qml
    qml/components/VoiceDock.qml
    qml/components/LeftProfileDock.qml
)

qt_add_executable(voxter_client
    ${CLIENT_SOURCES}
    ${CLIENT_HEADERS}
)

if (WIN32)
    set_target_properties(voxter_client PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

qt_add_qml_module(voxter_client
    URI Voxter
    VERSION 1.0
    QML_FILES
        ${CLIENT_QML}
)

target_link_libraries(voxter_client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Multimedia
    Qt6::WebSockets
    LiveKit::livekit
    livekit_ffi
)

target_include_directories(voxter_client PRIVATE src)

add_custom_command(TARGET voxter_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIVEKIT_SDK_DIR}/bin/livekit.dll"
        "$<TARGET_FILE_DIR:voxter_client>/livekit.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${LIVEKIT_SDK_DIR}/bin/livekit_ffi.dll"
        "$<TARGET_FILE_DIR:voxter_client>/livekit_ffi.dll"
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
