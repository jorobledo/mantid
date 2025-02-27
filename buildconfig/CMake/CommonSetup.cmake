# Include useful utils
include(MantidUtils)
include(GenerateMantidExportHeader)
# Make the default build type Release
if(NOT CMAKE_CONFIGURATION_TYPES)
  if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Release.")
    set(CMAKE_BUILD_TYPE
        Release
        CACHE STRING "Choose the type of build." FORCE
    )
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
  else()
    message(STATUS "Build type is " ${CMAKE_BUILD_TYPE})
  endif()
endif()

find_package(CxxTest)
if(CXXTEST_FOUND)
  add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})
  make_directory(${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Testing)
  message(STATUS "Added target ('check') for unit tests")
else()
  message(STATUS "Could NOT find CxxTest - unit testing not available")
endif()

# Avoid the linker failing by including GTest before marking all libs as shared and before we set our warning flags in
# GNUSetup
include(GoogleTest)
include(PyUnitTest)
enable_testing()

# build f2py fortran routines
if(ENABLE_F2PY_ROUTINES)
  include(f2pylibraries)
endif()

# We want shared libraries everywhere
set(BUILD_SHARED_LIBS On)

# This allows us to group targets logically in Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Look for stdint and add define if found
include(CheckIncludeFiles)
check_include_files(stdint.h stdint)
if(stdint)
  add_definitions(-DHAVE_STDINT_H)
endif(stdint)

# Configure a variable to hold the required test timeout value for all tests
set(TESTING_TIMEOUT
    300
    CACHE STRING "Timeout in seconds for each test (default 300=5minutes)"
)

option(ENABLE_OPENGL "Enable OpenGLbased rendering" ON)
option(ENABLE_OPENCASCADE "Enable OpenCascade-based 3D visualisation" ON)
option(USE_PYTHON_DYNAMIC_LIB "Dynamic link python libs" ON)

# ######################################################################################################################
# Look for dependencies Do NOT add include_directories commands here. They will affect every target.
# ######################################################################################################################
set(BOOST_VERSION_REQUIRED 1.65.0)
set(Boost_NO_BOOST_CMAKE TRUE)
# Unless specified see if the boost169 package is installed
if(EXISTS /usr/lib64/boost169 AND NOT (BOOST_LIBRARYDIR OR BOOST_INCLUDEDIR))
  message(STATUS "Using boost169 package in /usr/lib64/boost169")
  set(BOOST_INCLUDEDIR /usr/include/boost169)
  set(BOOST_LIBRARYDIR /usr/lib64/boost169)
endif()
find_package(Boost ${BOOST_VERSION_REQUIRED} REQUIRED COMPONENTS date_time regex serialization filesystem system)
add_definitions(-DBOOST_ALL_DYN_LINK -DBOOST_ALL_NO_LIB -DBOOST_BIND_GLOBAL_PLACEHOLDERS)
# Need this defined globally for our log time values
add_definitions(-DBOOST_DATE_TIME_POSIX_TIME_STD_CONFIG)
# Silence issues with deprecated allocator methods in boost regex
add_definitions(-D_SILENCE_CXX17_OLD_ALLOCATOR_MEMBERS_DEPRECATION_WARNING)

# if we are building the framework or mantidqt we need these
if(BUILD_MANTIDFRAMEWORK OR BUILD_MANTIDQT)
  find_package(Poco 1.4.6 REQUIRED)
  add_definitions(-DPOCO_ENABLE_CPP11)
  find_package(TBB REQUIRED)
  find_package(OpenSSL REQUIRED)
endif()

# if we are building the framework we will need these libraries.
if(BUILD_MANTIDFRAMEWORK)
  find_package(GSL REQUIRED)
  find_package(Nexus 4.3.1 REQUIRED)
  find_package(MuParser REQUIRED)
  find_package(JsonCPP 0.7.0 REQUIRED)

  if(ENABLE_OPENCASCADE)
    find_package(OpenCascade REQUIRED)
    add_definitions(-DENABLE_OPENCASCADE)
  endif()

  if(CMAKE_HOST_WIN32 AND NOT CONDA_ENV)
    find_package(ZLIB REQUIRED CONFIGS zlib-config.cmake)
    set(HDF5_DIR "${THIRD_PARTY_DIR}/cmake/hdf5")
    find_package(
      HDF5
      COMPONENTS C CXX HL
      REQUIRED CONFIGS hdf5-config.cmake
    )
    set(HDF5_LIBRARIES hdf5::hdf5_cpp-shared hdf5::hdf5_hl-shared)
  elseif(CONDA_ENV)
    # We'll use the cmake finder
    find_package(ZLIB REQUIRED)
    find_package(
      HDF5 MODULE
      COMPONENTS C CXX HL
      REQUIRED
    )
    set(HDF5_LIBRARIES hdf5::hdf5_cpp hdf5::hdf5)
    set(HDF5_HL_LIBRARIES hdf5::hdf5_hl)
  else()
    find_package(ZLIB REQUIRED)
    find_package(
      HDF5 MODULE
      COMPONENTS C CXX HL
      REQUIRED
    )
  endif()
endif()

find_package(Doxygen) # optional

# ######################################################################################################################
# Look for Git. Used for version headers - faked if not found. Also makes sure our commit hooks are linked in the right
# place.
# ######################################################################################################################

set(MtdVersion_WC_LAST_CHANGED_DATE Unknown)
set(MtdVersion_WC_LAST_CHANGED_DATETIME 0)
set(MtdVersion_WC_LAST_CHANGED_SHA Unknown)
set(MtdVersion_WC_LAST_CHANGED_BRANCHNAME Unknown)
set(NOT_GIT_REPO "Not")

if(GIT_FOUND)
  # Get the last revision
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_SHA_HEAD
    ERROR_VARIABLE NOT_GIT_REPO
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  if(NOT NOT_GIT_REPO) # i.e This is a git repository! "git describe" was originally used to produce this variable and
                       # this prefixes the short SHA1 with a 'g'. We keep the same format here now that we use rev-parse
    set(MtdVersion_WC_LAST_CHANGED_SHA "g${GIT_SHA_HEAD}")
    # Get the date of the last commit
    execute_process(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%cD
      OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_DATE
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    string(SUBSTRING ${MtdVersion_WC_LAST_CHANGED_DATE} 0 16 MtdVersion_WC_LAST_CHANGED_DATE)

    execute_process(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%H
      OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_SHA_LONG
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )

    # getting the datetime (as iso8601 string) to turn into the patch string
    execute_process(
      COMMAND ${GIT_EXECUTABLE} log -1 --format=format:%ci
      OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_DATETIME
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    if(MtdVersion_WC_LAST_CHANGED_DATETIME)
      # split into "date time timezone"
      string(REPLACE " " ";" LISTVERS ${MtdVersion_WC_LAST_CHANGED_DATETIME})
      list(GET LISTVERS 0 ISODATE)
      list(GET LISTVERS 1 ISOTIME)
      list(GET LISTVERS 2 ISOTIMEZONE)

      # turn the date into a number
      string(REPLACE "-" "" ISODATE ${ISODATE})

      # prepare the time
      string(REGEX REPLACE "^([0-9]+:[0-9]+).*" "\\1" ISOTIME ${ISOTIME})
      string(REPLACE ":" "" ISOTIME ${ISOTIME})

      # convert the timezone into something that can be evaluated for math
      if(ISOTIMEZONE STREQUAL "+0000")
        set(ISOTIMEZONE "") # GMT do nothing
      else()
        string(SUBSTRING ${ISOTIMEZONE} 0 1 ISOTIMEZONESIGN)
        if(ISOTIMEZONESIGN STREQUAL "+")
          string(REPLACE "+" "-" ISOTIMEZONE ${ISOTIMEZONE})
        else()
          string(REPLACE "-" "+" ISOTIMEZONE ${ISOTIMEZONE})
        endif()
      endif()

      # remove the timezone from the time to convert to GMT
      math(EXPR ISOTIME "${ISOTIME}${ISOTIMEZONE}")

      # deal with times crossing midnight this does not get the number of days in a month right or jan 1st/dec 31st
      if(ISOTIME GREATER 2400)
        math(EXPR ISOTIME "${ISOTIME}-2400")
        math(EXPR ISODATE "${ISODATE}+1")
      elseif(ISOTIME LESS 0)
        math(EXPR ISOTIME "2400${ISOTIME}")
        math(EXPR ISODATE "${ISODATE}-1")
      endif()

      set(MtdVersion_WC_LAST_CHANGED_DATETIME "${ISODATE}.${ISOTIME}")
    endif()

    # conda builds want to know about the branch being used otherwise the variable is "Unknown"
    if(ENABLE_CONDA)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} name-rev --name-only HEAD
        OUTPUT_VARIABLE MtdVersion_WC_LAST_CHANGED_BRANCHNAME
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      )
    endif()

    # ##################################################################################################################
    # This part puts our hooks (in .githooks) into .git/hooks
    # ##################################################################################################################
    # First need to find the top-level directory of the git repository
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --show-toplevel
      OUTPUT_VARIABLE GIT_TOP_LEVEL
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    # N.B. The variable comes back from 'git describe' with a line feed on the end, so we need to lose that
    string(REGEX MATCH "(.*)[^\n]" GIT_TOP_LEVEL ${GIT_TOP_LEVEL})
    # Prefer symlinks on platforms that support it so we don't rely on cmake running to be up-to-date On Windows, we
    # have to copy the file
    if(WIN32)
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${GIT_TOP_LEVEL}/.githooks/commit-msg ${GIT_TOP_LEVEL}/.git/hooks
      )
    else()
      execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${GIT_TOP_LEVEL}/.githooks/commit-msg
                ${GIT_TOP_LEVEL}/.git/hooks/commit-msg
      )
    endif()

  endif()

else()
  # Just use a dummy version number and print a warning
  message(STATUS "Git not found - using dummy revision number and date")
endif()

mark_as_advanced(MtdVersion_WC_LAST_CHANGED_DATE MtdVersion_WC_LAST_CHANGED_DATETIME)

if(NOT NOT_GIT_REPO) # i.e This is a git repository!
                     # #################################################################################################
                     # Create the file containing the patch version number for use by cpack The patch number make have
                     # been overridden by VersionNumber so create the file used by cpack here
                     # #################################################################################################
  configure_file(
    ${GIT_TOP_LEVEL}/buildconfig/CMake/PatchVersionNumber.cmake.in
    ${GIT_TOP_LEVEL}/buildconfig/CMake/PatchVersionNumber.cmake
  )
  include(PatchVersionNumber)
endif()

# ######################################################################################################################
# Include the file that contains the version number This must come after the git business above because it can be used
# to override the patch version number
# ######################################################################################################################
include(VersionNumber)

# ######################################################################################################################
# Look for OpenMP
# ######################################################################################################################
find_package(OpenMP COMPONENTS CXX)
if(OpenMP_CXX_FOUND)
  link_libraries(OpenMP::OpenMP_CXX)
endif()

# ######################################################################################################################
# Add linux-specific things
# ######################################################################################################################
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  include(LinuxSetup)
endif()

# ######################################################################################################################
# Set the c++ standard to 17 - cmake should do the right thing with msvc
# ######################################################################################################################
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ######################################################################################################################
# Add compiler options if using gcc
# ######################################################################################################################
if(CMAKE_COMPILER_IS_GNUCXX)
  include(GNUSetup)
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
  include(GNUSetup)
endif()

# ######################################################################################################################
# Configure clang-tidy if the tool is found
# ######################################################################################################################
include(ClangTidy)

# ######################################################################################################################
# Setup cppcheck
# ######################################################################################################################
include(CppCheckSetup)

# ######################################################################################################################
# Setup pylint
# ######################################################################################################################
include(PylintSetup)

# ######################################################################################################################
# Set up the unit tests target
# ######################################################################################################################

# ######################################################################################################################
# External Data for testing
# ######################################################################################################################
if(CXXTEST_FOUND OR PYUNITTEST_FOUND)
  include(SetupDataTargets)
endif()

# ######################################################################################################################
# Visibility Setting
# ######################################################################################################################
if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_VISIBILITY_PRESET
      hidden
      CACHE STRING ""
  )
endif()

# ######################################################################################################################
# Bundles setting used for install commands if not set by something else e.g. Darwin
# ######################################################################################################################
if(NOT BUNDLES)
  set(BUNDLES "./")
endif()

# ######################################################################################################################
# Set an auto generate warning for .in files.
# ######################################################################################################################
set(AUTO_GENERATE_WARNING "/********** PLEASE NOTE! THIS FILE WAS AUTO-GENERATED FROM CMAKE.  ***********************/")

# ######################################################################################################################
# Setup pre-commit here as otherwise it will be overwritten by earlier pre-commit hooks being added
# ######################################################################################################################
option(ENABLE_PRECOMMIT "Enable pre-commit framework" ON)
if(ENABLE_PRECOMMIT)
  # Windows should use downloaded ThirdParty version of pre-commit.cmd Everybody else should find one in their PATH
  find_program(
    PRE_COMMIT_EXE
    NAMES pre-commit
    HINTS ~/.local/bin/ "${MSVC_PYTHON_EXECUTABLE_DIR}/Scripts/"
  )
  if(NOT PRE_COMMIT_EXE)
    message(FATAL_ERROR "Failed to find pre-commit see https://developer.mantidproject.org/GettingStarted.html")
  endif()

  if(WIN32)
    if(CONDA_ENV)
      execute_process(
        COMMAND "${PRE_COMMIT_EXE}" install --overwrite
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE PRE_COMMIT_RESULT
      )
    else()
      execute_process(
        COMMAND "${PRE_COMMIT_EXE}.cmd" install --overwrite
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        RESULT_VARIABLE PRE_COMMIT_RESULT
      )
    endif()
    if(NOT PRE_COMMIT_RESULT EQUAL "0")
      message(FATAL_ERROR "Pre-commit install failed with ${PRE_COMMIT_RESULT}")
    endif()
    # Create pre-commit script wrapper to use mantid third party python for pre-commit
    if(NOT CONDA_ENV)
      file(RENAME "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit" "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit-script.py")
      file(
        WRITE "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit"
        "#!/usr/bin/env sh\n${MSVC_PYTHON_EXECUTABLE_DIR}/python.exe ${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit-script.py"
      )
    else()
      file(TO_CMAKE_PATH $ENV{CONDA_PREFIX} CONDA_SHELL_PATH)
      file(RENAME "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit" "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit-script.py")
      file(
        WRITE "${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit"
        "#!/usr/bin/env sh\n${CONDA_SHELL_PATH}/Scripts/wrappers/conda/python.bat ${PROJECT_SOURCE_DIR}/.git/hooks/pre-commit-script.py"
      )
    endif()
  else() # linux as osx
    execute_process(
      COMMAND bash -c "${PRE_COMMIT_EXE} install"
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
      RESULT_VARIABLE STATUS
    )
    if(STATUS AND NOT STATUS EQUAL 0)
      message(
        FATAL_ERROR
          "Pre-commit tried to install itself into your repository, but failed to do so. Is it installed on your system?"
      )
    endif()
  endif()
else()
  message(AUTHOR_WARNING "Pre-commit not enabled by CMake, please enable manually.")
endif()

# ######################################################################################################################
# Set a flag to indicate that this script has been called
# ######################################################################################################################

set(COMMONSETUP_DONE TRUE)
