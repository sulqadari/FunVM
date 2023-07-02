echo on
if exist build rmdir /s /q build
mkdir build
cd build
cmake -GNinja ../
cmake --build .