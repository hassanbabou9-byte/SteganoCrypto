# Shared, GUI-independent code: crypto, password hashing, steganography.
CONFIG += c++17

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/chiffrement.h \
    $$PWD/passwordhasher.h \
    $$PWD/steganography.h

SOURCES += \
    $$PWD/chiffrement.cpp \
    $$PWD/passwordhasher.cpp \
    $$PWD/steganography.cpp

# ---- OpenSSL ---------------------------------------------------------------
# Linux / macOS: found through pkg-config (macOS/Homebrew: export
#   PKG_CONFIG_PATH="$(brew --prefix openssl@3)/lib/pkgconfig").
# Windows: set the OPENSSL_DIR environment variable (or pass OPENSSL_DIR=... to
#   qmake) to your OpenSSL install; defaults to the standard installer location.
unix {
    CONFIG += link_pkgconfig
    PKGCONFIG += openssl
}
win32 {
    isEmpty(OPENSSL_DIR): OPENSSL_DIR = $$(OPENSSL_DIR)
    isEmpty(OPENSSL_DIR): OPENSSL_DIR = "C:/Program Files/OpenSSL-Win64"
    INCLUDEPATH += "$$OPENSSL_DIR/include"
    LIBS += -L"$$OPENSSL_DIR/lib/VC/x64/MD" -lssl -lcrypto
}
