export LDFLAGS="-L/usr/local/opt/binutils/lib"
export CPPFLAGS="-I/usr/local/opt/binutils/include"
cd projects/unix
# make clean
make all DEBUGGER=1 DEBUG=1 && cp libmupen64plus.dylib ../../../ali\ mupen64_31october18/unix/