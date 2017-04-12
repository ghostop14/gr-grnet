# gr-grnet
GNURadio TCP and UDP sink blocks completely rewritten in C++ with Boost.  The UDP block has some additional header/crc options that can be optionally enabled as well:

Header/CRC

None (raw byte stream)

leading 32-bit sequence number

leading 32-bit sequence number and 32-bit data size

leading sequence # and data size + trailing crc32



## Building
Build is pretty standard:

mkdir build

cd build

cmake ..

make

make install

ldconfig

## Performance
Throughput is always important.  Each was measured against a local 127.0.0.1 listener on port 2000.  The following metrics were generated (the module installs a test-netsink command-line tool that can be used to test your own performance.

TCP Sink

Testing netsink.  Sending test data to 127.0.0.1:2000

Code Run Time:      0.000003 s  (3045024768.000000 Bps)


UDP Sink

Testing netsink.  Sending test data to 127.0.0.1:2000

Code Run Time:      0.000004 s  (1979805696.000000 Bps)


