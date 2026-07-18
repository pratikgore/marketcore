#Build commands 

//From marketcore root

cmake -S Utils -B build/utils -DCMAKE_BUILD_TYPE=Release
cmake --build build/utils -j