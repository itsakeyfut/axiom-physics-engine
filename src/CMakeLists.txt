# Axiom source directory

# Core modules (Phase 1)
add_subdirectory(core)
add_subdirectory(math)
add_subdirectory(memory)

# GPU module (Phase 2)
add_subdirectory(gpu)

# Debug module (Phase 2 - Debug visualization)
add_subdirectory(debug)

# Frontend module (Phase 2 - Window management)
add_subdirectory(frontend)

# Application (main executable)
add_subdirectory(app)

# Future modules (will be uncommented as they are implemented):
# add_subdirectory(collision)
# add_subdirectory(dynamics)
# add_subdirectory(softbody)
# add_subdirectory(fluid)

message(STATUS "Axiom modules configured: core, math, memory, gpu, debug, frontend")
