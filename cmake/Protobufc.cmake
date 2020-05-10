find_path(PROTOBUFC_ROOT_DIR
        NAMES include/protobuf-c/protobuf-c.h
        PATHS /usr/local/ /usr/local/opt/protobuf-c/ /opt/local/ /usr/
        NO_DEFAULT_PATH
        )
find_path(PROTOBUFC_DIR
        NAMES include/protobuf-c/protobuf-c.h
        PATHS ${PROTOBUFC_ROOT_DIR}/include
        NO_DEFAULT_PATH
        )

find_path(PROTOBUFC_INCLUDE_DIR
        NAMES protobuf-c/protobuf-c.h
        PATHS ${PROTOBUFC_ROOT_DIR}/include
        NO_DEFAULT_PATH
        )

find_library(PROTOBUFC_LIBRARY
        NAMES protobuf-c
        PATHS ${PROTOBUFC_ROOT_DIR}/lib /usr/lib/x86_64-linux-gnu
        NO_DEFAULT_PATH
        )

if(PROTOBUFC_INCLUDE_DIR AND PROTOBUFC_LIBRARY)
    set(PROTOBUFC_FOUND TRUE)
else()
    set(PROTOBUFC_FOUND FALSE)
endif()

mark_as_advanced(
        PROTOBUFC_ROOT_DIR
        PROTOBUFC_INCLUDE_DIR
        PROTOBUFC_LIBRARY
)

message("PROTOBUFC_ROOT_DIR found ${PROTOBUFC_ROOT_DIR}")
message("PROTOBUFC_INCLUDE_DIR found ${PROTOBUFC_INCLUDE_DIR}")
message("PROTOBUFC_LIBRARY found ${PROTOBUFC_LIBRARY}")
message("PROTOBUFC_FOUND found ${PROTOBUFC_FOUND}")

set(CMAKE_REQUIRED_INCLUDES ${PROTOBUFC_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${PROTOBUFC_LIBRARY})

