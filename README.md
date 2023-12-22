# net.http
Simple HTTP interface written in modern C++

# build
```bash
git clone https://github.com/peixy0/net.http
cd net.http
mkdir externals
git clone https://github.com/google/googletest externals/googletest
git clone https://github.com/gabime/spdlog externals/spdlog
mkdir build
cd build
cmake .. -GNinja
ninja
```

# example
see src/main
