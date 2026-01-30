# Axiom source directory

# Core modules (Phase 1)
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(memory)

# Future modules (will be uncommented as they are implemented):
# add_subdirectory(collision)
# add_subdirectory(dynamics)
# add_subdirectory(softbody)
# add_subdirectory(fluid)
# add_subdirectory(gpu)

message(STATUS "Axiom modules configured: core, math, memory")
