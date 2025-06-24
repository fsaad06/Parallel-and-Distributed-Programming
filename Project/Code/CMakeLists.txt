add_executable(MPILaplace MPILaplace.cpp)
set_target_properties(MPILaplace PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)

find_package(MPI REQUIRED)
if(NOT MPI_CXX_FOUND)
message("MPI not found. please install an mpi implementation, for example:")
message("Windows: https://learn.microsoft.com/en-us/message-passing-interface/microsoft-mpi")
message("Linux: sudo apt install openmpi-bin openmpi-common libopenmpi-dev")
endif()
target_link_libraries(MPILaplace PRIVATE MPI::MPI_CXX)

find_package(OpenMP REQUIRED)
target_link_libraries(MPILaplace PRIVATE OpenMP::OpenMP_CXX)

if("${MPI_CXX_INCLUDE_DIRS}" MATCHES "hpcx")
message("nvidia HPC-X found")
set(NVIDIA_HPCX TRUE)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
if(NOT MSVC)
if(${NVIDIA_HPCX})
add_custom_target(run COMMAND mpirun -n 4 -mca coll_hcoll_enable 0 ./MPIHelloWorld DEPENDS MPILaplace)
else()
add_custom_target(run COMMAND mpirun -n 4 ./MPILaplace DEPENDS MPIMatMult)
endif()
endif()

# Command: $(MSMPI_BIN)\mpiexec
# Command arguments: -n 4 "$(TargetPath)"
set_target_properties(MPILaplace PROPERTIES VS_DEBUGGER_COMMAND           "$(MSMPI_BIN)mpiexec"
                                              VS_DEBUGGER_COMMAND_ARGUMENTS "-n 5 ..\\src\\$(Configuration)\\MPILaplace")
											  
