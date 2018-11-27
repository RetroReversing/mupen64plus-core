export LDFLAGS="-L/usr/local/opt/binutils/lib"
export CPPFLAGS="-I/usr/local/opt/binutils/include"
make all DEBUGGER=1 && cp libmupen64plus.dylib ../../../ali\ mupen64_31october18/unix/