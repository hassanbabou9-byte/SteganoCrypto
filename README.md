# SteganoCrypto

A small Qt 6 desktop app that **encrypts a text message and hides it inside an image**.
Only someone with the passphrase *and* the image can read it back.

1. The message is encrypted with **AES-256-GCM** (key derived from your passphrase with PBKDF2-HMAC-SHA256, random salt + IV per message).
2. The encrypted bytes are hidden in the least-significant bits of the image's R, G and B channels (**LSB steganography**, 3 bits per pixel).
3. Save the result as **PNG or BMP** and share it. Load it again, enter the passphrase, and the message is recovered. A wrong passphrase or a tampered image is detected, never silently decoded to garbage.

The app also has a local sign-up / sign-in screen backed by SQLite. Passwords are stored as salted PBKDF2-SHA256 hashes, never in clear text.

## Build

Requirements: Qt 6 (Widgets, SQL with the SQLite driver), OpenSSL 3 development files, a C++17 compiler, qmake.

**Linux (Debian/Ubuntu)**
```bash
sudo apt install qt6-base-dev libqt6sql6-sqlite libssl-dev
mkdir build && cd build
qmake6 ../SteganoCrypto.pro
make -j
./app/SteganoCrypto
```

**macOS (Homebrew)**
```bash
brew install qt openssl@3
export PKG_CONFIG_PATH="$(brew --prefix openssl@3)/lib/pkgconfig"
mkdir build && cd build && qmake ../SteganoCrypto.pro && make -j
```

**Windows**
Install Qt 6 with an **MSVC 64-bit kit**, the matching Visual Studio C++ build tools, and OpenSSL 3 for Windows (for example, the Shining Light installer). The MSVC kit is required because the project links against OpenSSL's MSVC libraries.

From an **x64 Native Tools Command Prompt for Visual Studio**, in the project root:
Ensure the selected Qt MSVC kit's `bin` directory is on `PATH` so `qmake` uses that kit.
```bat
set "OPENSSL_DIR=C:/Program Files/OpenSSL-Win64"
mkdir build
cd build
qmake ..\SteganoCrypto.pro
nmake
```
Run `app\release\SteganoCrypto.exe` from the build directory. The OpenSSL runtime DLLs (`libssl-3-x64.dll` and `libcrypto-3-x64.dll`) must be on `PATH` when launching it. For development, you can instead open `SteganoCrypto.pro` in Qt Creator and select the matching MSVC kit.

## Tests

```bash
cd build
QT_QPA_PLATFORM=offscreen make check
```
Covers encryption (round trip, wrong key, tampering, randomisation), password hashing, steganography
(round trip, capacity, PNG/BMP persistence, transparency, >64 KiB payloads), the user database, and a
GUI test that drives the real window.

## Project layout

```
core/    crypto, password hashing, steganography (no GUI dependencies)
app/     Qt Widgets application: login, sign-up, main window, user database
tests/   QtTest suites (core + GUI)
```

## Usage tips and limitations

- **Use PNG or BMP.** JPEG is lossy: hidden data does not survive saving as JPEG, resizing, or most messaging apps that recompress images.
- **Keep your passphrase.** Without it the message cannot be recovered. Leave the field empty to get a strong random one generated for you.
- **Capacity:** about 3 bits per opaque pixel, so a 1000x1000 image holds roughly 375 KB.
- **LSB steganography is not undetectable.** It hides a message from casual viewers, but statistical analysis can reveal that an image has been modified. The *encryption* is what keeps the content confidential.
- The sign-in screen is a local convenience gate for the app's UI. It is not a security boundary: anyone with access to the machine could run the encryption code without it.

