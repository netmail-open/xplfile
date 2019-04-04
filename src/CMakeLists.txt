add_library(xplfile SHARED
  file.c
)


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ALL_CFLAGS} ${PTHREAD_CFLAGS} ${OPENSSL_CFLAGS}")

find_library(MEMMGR_LIBRARY
  NAMES libmemmgr memmgr
)

target_link_libraries(xplfile
	memmgr${BIT}
)

install(TARGETS xplfile DESTINATION /usr/lib)
