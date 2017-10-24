# logger
## Intro
logger is a C++ light weight yet fast log library.

## Features:
- Multi-threaded supported
- Modern C++-style stream API or old school function style API
- Log level is supported(`DEBUG, INFO, WARN, ERROR, FATAL`)
- Log rolling only supported by time interval(`ONE_HOUR, TWO_HOURS, HALF_DAY, A_DAY`)
- Multiple log files simultaneously

## Upcoming features:
- Log rolling by size

## Build

g++ test.cpp -std=c++11 -I. ./build/liblogger.a -lpthread
