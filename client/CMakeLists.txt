# Build the libctwm_client library
add_library(ctwmc SHARED libctwm.c)

# There's a demolib that links against it
add_executable(demolib demolib.c)
target_link_libraries(demolib ctwmc ${CTWMLIBS})

# Mention it
message(STATUS "Building libctwmc and demolib.")
