# Prebuilt Pico SDK host tools

Place the official Windows prebuilt Pico SDK host tools in this directory when
building on Windows without a host C/C++ compiler installed:

- `pioasm.exe`
- `picotool.exe`

Download these executables from the Raspberry Pi Foundation `pico-sdk-tools`
releases. The repository intentionally does not include the binaries.

By default, Windows CMake configure uses these paths when
`USE_PREBUILT_PICO_TOOLS` is `ON`. If you want Pico SDK to build the host tools
from source instead, configure with `-DUSE_PREBUILT_PICO_TOOLS=OFF`.
