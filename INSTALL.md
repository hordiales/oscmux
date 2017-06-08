
# Build

            $ cmake CMakeLists.txt
            $ make
            
# Some extra dependencies
 
## MAC
            $ sudo port install liblo eina
            
## Linux

            $ sudo apt-get install -y \
                        build-essential \
                        cmake \
                        liblo-dev \
                        libeina-dev

WARNING: Workaround if Efl_Config.h is missing. Add '/usr/include/efl-1' or ${PC_EINA_INCLUDE_DIRS} path to CMakeLists.txt

            #include_directories (${EINA_INCLUDE_DIR})
            include_directories (${PC_EINA_INCLUDE_DIRS})
            #include_directories (/usr/include/efl-1)
