CONFIG += debug
LIBS += -lunicorn -lpthread  -lstdc++fs
HEADERS += main.h dlls/kernel32.h dlls/user32.h dlls/gdi32.h
SOURCES += main.cpp dlls/kernel32.cpp dlls/user32.cpp dlls/gdi32.cpp
