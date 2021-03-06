# logger
[![LICENSE](https://img.shields.io/badge/License-MIT-red)](./LICENSE)
## Intro
logger is a C++ light weight yet fast log library.

## Features:
- Multi-threaded supported
- Modern C++-style stream API or old school function style API
- Log level is supported(`DEBUG, INFO, WARN, ERROR, FATAL`)
- Log rotating by time interval, file size, file lines are supported
- Multiple log files simultaneously
- Support IO options, log data will be flushed to file according to sink type parameters (`BY_LINE, BY_BUF_SIZE`)

## Updates:

### 20200403 
- Support IO options 

### 20190517
- Milliseconds in log timestamp supported
- Custom prefix in log supported

## Supported platform
- Unix/linux
- others are not tested

## Upcoming features:

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
g++ test.cpp -std=c++11 -I.. ../build/liblogger.a -lpthread
```
