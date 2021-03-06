set(SOURCE_FILES_SNI
        http.c
        tls.c
        rule.c)

set(SOURCE_FILES_OBFS
        obfs/verify.c
        obfs/verify.h
        obfs/tls1.2_ticket.c
        obfs/tls1.2_ticket.h
        obfs/obfsutil.c
        obfs/obfsutil.h
        obfs/obfs.c
        obfs/obfs.h
        obfs/http_simple.c
        obfs/http_simple.h
        obfs/crc32.c
        obfs/crc32.h
        obfs/base64.c
        obfs/base64.h
        obfs/auth_chain.c
        obfs/auth_chain.h
        obfs/auth.c
        obfs/auth.h)

set(SOURCE_FILES_LOCAL
        android.c
        getopt.c
        getopt.h
        utils.c
        jconf.c
        json.c
        encrypt.c
        udprelay.c
        cache.c
        acl.c
        netutils.c
        local.c
        ${SOURCE_FILES_OBFS}
        ${SOURCE_FILES_SNI})

set(SOURCE_FILES_TUNNEL
        utils.c
        jconf.c
        json.c
        encrypt.c
        udprelay.c
        cache.c
        netutils.c
        tunnel.c)

set(SOURCE_FILES_SERVER
        utils.c
        jconf.c
        json.c
        encrypt.c
        udprelay.c
        cache.c
        acl.c
        resolv.c
        netutils.c
        server.c
        ${SOURCE_FILES_OBFS}
        ${SOURCE_FILES_SNI})

set(SOURCE_FILES_MANAGER
        utils.c
        jconf.c
        json.c
        manager.c)

set(SOURCE_FILES_REDIR
        utils.c
        jconf.c
        json.c
        encrypt.c
        netutils.c
        cache.c
        udprelay.c
        redir.c
        ${SOURCE_FILES_OBFS}
        ${SOURCE_FILES_SNI})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${libev_include_dirs})
include_directories(${libcork_include_dirs})
include_directories(${libipset_include_dirs})
include_directories(${libudns_include_dirs})
include_directories(${libsodium_include_dirs})


if (WIN32)
    list ( APPEND SOURCE_FILES_LOCAL win32.c )
    list ( APPEND SOURCE_FILES_TUNNEL win32.c )
    list ( APPEND SOURCE_FILES_SERVER win32.c )
endif ()

if (ANDROID)
    include(FetchContent)
    FetchContent_Declare(fake-dlfcn
            GIT_REPOSITORY https://github.com/ssrlive/fake-dlfcn.git
            )
    FetchContent_MakeAvailable(fake-dlfcn)

    add_definitions(-DHAVE_CONFIG_H -DUSE_CRYPTO_MBEDTLS -DCONNECT_IN_PROGRESS=EINPROGRESS -DHAVE_PCRE_H)
    if (BUILD_SSR_CLIENT_AS_EXE)
    add_executable(ssr-client ${SOURCE_FILES_LOCAL})
    else()
    add_library(ssr-client SHARED ${SOURCE_FILES_LOCAL})
    endif(BUILD_SSR_CLIENT_AS_EXE)
    set_target_properties(ssr-client PROPERTIES COMPILE_DEFINITIONS MODULE_LOCAL)
    set_target_properties(ssr-client PROPERTIES PREFIX "lib" SUFFIX ".so")
    target_link_libraries(ssr-client
            m log
            ev libcork libipset udns sodium
            ${LIBCRYPTO} ${PCRE_LIBRARIES}
            fake-dlfcn
            )
	target_include_directories(ssr-client PRIVATE ${LIB_FAKE_DLFCN_DIR})
else()

add_executable(ssr-client ${SOURCE_FILES_LOCAL})
#add_executable(ss_tunnel ${SOURCE_FILES_TUNNEL})
#add_executable(ss_server ${SOURCE_FILES_SERVER})
#add_executable(ss_manager ${SOURCE_FILES_MANAGER})
add_executable(ss_redir ${SOURCE_FILES_REDIR})
#add_library(libshadowsocks-libev ${SOURCE_FILES_LOCAL})

set_target_properties(ssr-client PROPERTIES
        COMPILE_DEFINITIONS MODULE_LOCAL
        )
#set_target_properties(ss_tunnel PROPERTIES
#        COMPILE_DEFINITIONS MODULE_TUNNEL
#        )
#set_target_properties(ss_server PROPERTIES
#        COMPILE_DEFINITIONS MODULE_REMOTE
#        )
#set_target_properties(ss_manager PROPERTIES
#        COMPILE_DEFINITIONS MODULE_MANAGER
#        )
set_target_properties(ss_redir PROPERTIES
        COMPILE_DEFINITIONS MODULE_REDIR
        )
#target_compile_definitions(libshadowsocks-libev PRIVATE MODULE_LOCAL LIB_ONLY)
#set_target_properties(libshadowsocks-libev PROPERTIES SUFFIX ".la")

set (ss_lib_common
        ev
        m
        libcork)

set (ss_lib_net
        ${ss_lib_common}
        ${LIBCRYPTO}
        libipset
        ${PCRE_LIBRARIES}
        udns
        sodium
        )

if (WIN32)
    list ( APPEND ss_lib_net Ws2_32 )
endif()

target_link_libraries(ssr-client
        ${ss_lib_net}
)
#target_link_libraries(ss_tunnel
#        ${ss_lib_net}
#        )
#target_link_libraries(ss_server
#        ${ss_lib_net}
#        )
#target_link_libraries(ss_manager
#        ${ss_lib_common}
#        )
target_link_libraries(ss_redir
        ${ss_lib_net}
        )

endif()
