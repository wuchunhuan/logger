# logger
## Intro
logger is a C++ light weight yet fast log library.

## Features:
- Multi-threaded supported
- Modern C++-style stream API or old school function style API
- Log level is supported(`DEBUG, INFO, WARN, ERROR, FATAL`)
- Log rolling only supported by time interval(`ONE_HOUR, TWO_HOURS, HALF_DAY, A_DAY`)
- Multiple log files simultaneously

## Supported platform
- Unix/linux
- others not tested

## Upcoming features:
- Log rolling by size

## Build
### Prerequisites
- cmake
- c++11

### Compile library
```
git clone https://github.com/heroperseus/logger.git

cd logger
mkdir build && cd build
cmake ..
make
sudo make install
```
### Linking library
```C++
g++ test.cpp -std=c++11 -I. ./build/liblogger.a -lpthread
```
