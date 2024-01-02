# Hardware
Tested on my main machine which is running `Fedora 39(x86_64)` with `g++-13.2.1` and `boost-devel-1.84.0`, and on my other
machine with `Ubuntu 20.04 LTS(x86_64)` with `g++-10.5.0` and `libboost-devel-1.71.0`

The minimal compiler(GCC) version is 10.0. Speaking about other compilers, well, every one that has c++20 standard support, especially
`concept` thing is suitable.
The minimal version of `Boost` is `1.71.0` or less.

### Setting up
To run this application you need:
 * CMake
 * Python
 * Boost Library

To install `boost` library on RPM-based GNU/Linux distros proceed with:
```
$ sudo dnf install boost-devel
```
On Debian-based GNU/Linux distros:
```
$ sudo apt-get install libboost-all-dev
```

Next, `cd` into the `build` directory, and specify pre-defined compilers macros, as follows:
```
$ cmake -DCLIENTS_THREAD_POOL_CAPACITY=1000 -DOZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES=5000000 -DOZZY_USE_LARGE_CHUNK_MEMORY_ARENAS=1 ..
```
Or! just type this to use default values
```
$ cmake ..
```

The `CLIENTS_THREAD_POOL_CAPACITY` - the maximum simultanious connections that server can hold. Setting this value to `<1` makes program
behaviour undefined.

The `OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES` - the size of the memory arena that is reserved for processing one chunk of data, the more value is
the faster the chunk will be processed.

The `OZZY_USE_LARGE_CHUNK_MEMORY_ARENAS` - by default, the server has a memory cap that can be reserved for one chunk(because we don't want to
reserve `1Gb` of memory to process `1Mb` chunk). This was added because we don't know what the chunk size will be. If we are really sure that we have as
much data transmitting per session as specified in `OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES`, we could set this flag not to cap the memory that reserved for
processing one chunk.

Then just build:
```
$ cmake --build .
```

## Running the program
Open two terminal instances, in the first one launch the server and specify the amount of doubles that will be sended per session
```
$ ./ozzy_server --doubles 1000000
```
In the other one, execute the client(inside there's actually 4 clients running in separate threads) an specify the X value that is gonna be used as the upper and lower bound
for the generated set of doubles in the payload. The default value for `--doubles` if not explicitly specified is: **1000000(one million)**

```
$ ./ozzy_client --x 20
```
After the program is finished, you might want to look at the resulting file, so i wrote a simple python script that parses the generated files. To execute it enter:
```
$ python3 hextodouble.py 
```
The default value for `--x` if not explicitly specified is: **1.0**. It will read the `result.bin` file and show you it's contents in a human-readable format

*WARNING: This script works only on little endian machines!*

### Protocol Overview
General rules of the protocol:
  * Each object should be less or equal size of the MTU of `1500` bytes
  * Each object should contain unique identifier for this object(f.e: `MESSAGE_TYPE_FRAME`) with the size of `1` octet

The `Ozzy::Frame` object consists of:
  * Type of the message, that will be interpreted by the server
  * Length of the payload
  * Checksum of the payload, to validate the packet on the client side
  * Payload with doubles

Type and length are `2` octets, checksum is the `4`, Total of `8` octets for the 
frame header. The idea is that we have kinda dynamic checksum size, and if
we want to add more fields to the header(for example `1` bit `is_encrypted` flag)
we will not drop the payload offset, and just borrow bits from the checksum bitfield.


```
                        Frame header(always 8 octets)
+----------------------+---------------------------+---------------------------+-------------------------+
|       Field          |        Octets             |        Description        |     Constant value      |
+----------------------+---------------------------+---------------------------+-------------------------+
|       type           |         0-0               |  Type of this message     |   MESSAGE_TYPE_FRAME(0) | 
|                      |                           |                           |                         |
+----------------------+---------------------------+---------------------------+-------------------------+
|      length          |         1-1               |  Maximum length of the    |
|                      |                           |  packet payload.          |
+----------------------+---------------------------+---------------------------+
|     checksum         |         2-7               |  Bits for the checksum    |
|                      |                           |  of the payload.          |
+----------------------+---------------------------+---------------------------+

               Frame payload(1400 octets *maximum* in this implementation)
+----------------------+---------------------------+---------------------------+
|     payload          |      8-1407               |  Payload with doubles.    |
|                      |                           |                           |
+----------------------+---------------------------+---------------------------+
```

The `Ozzy::Handshake` object consists of:
 * Endian control value, to find out which architecture uses the client
 * Type of the message, that will be interpreted by the server

```
                        Handshake(always 16 octets)
+----------------------+---------------------------+---------------------------+--------------------------+
|       Field          |    Octets (LSB-MSB)       |        Description        |     Constant value       |
+----------------------+---------------------------+---------------------------+--------------------------+
|       endian         |   0 - 7                   |  Endian control value     |       0x10 or 0x01       |
|                      |                           |                           |                          |
+----------------------+---------------------------+---------------------------+--------------------------+
|       type           |   8 - 15                  |  Type of this message     | MESSAGE_TYPE_HANDSHAKE(1)|
|                      |                           |                           |                          |
+----------------------+---------------------------+---------------------------+--------------------------+
```

The `Ozzy::Answer` object is a `8` octet size value, that transmits between server and client.

The `Ozzy::v1::Answer` can hold up to `10` distinct values:
* `ACK`  - Acknowledged, used as a flag that data transmitted succesfully
* `NACK` - Not Acknowledged, used as a flag that data transmittion failed
* `DROP` - Drop the connection
* `CONT` - Continue sending frames(sended after each frame, indicating that there at least one more frame needs to be sended)
* `BRK`  - Break sending frames(sended after each frame, indicating that there no more frames needs to be sended)
* `ERR_VERSIONS_INCOMPATIBLE` - Incompatible protocol versions between server and client

The `Ozzy::v2::Answer` can hold up to 10 distinct values:
* `CLIENT_THREAD_POOL_EXHAUSED` - Simultanious connections reached their's limit on the server

### Chunk processing
Each thread is writing the frame data to the specific thread cache file. The name of the file is 128 char-wide(from numeric+symbolic alphabet)
with extensions of `_thread_cache.bin`.
After the file writing is finished, the application(client connection thread to be more specific) allocated the memory of size `OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES`(or less, look above), 
loads the contents of the thread cache file into this memory segment, sorts it and writes to the same-generated cache files, but with postfix of `_thread_chunk.bin`
After all chunks has been sorted, the application merges them into the final result file, that has the header `THREAD_CACHE_MAGIC(0x595A5A4F -- OZZY)` guarding them by 
`THREAD_CACHE_START_H(0xDEADBEEF)` at start and `THREAD_CACHE_END_H(0xC0FFEE)` at the end. 

So, the algorithm looks like this:
 * Write all received frames to the `*_thread_cache.bin` file;
 * Load the chunk of this file to the memory one by one(chunk size is `OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES`);
 * Sort it;
 * Write to the `*_thread_chunk.bin` file;
 * After all chunks has been sorted and written, create `thread cache file size / OZZY_CHUNK_MEMORY_ARENA_SIZE_BYTES` chunk readers;
 * Find the least value between all this readers at the position of theirs file pointer;
 * Move the file pointer of the reader with minimal value `sizeof(double)` size;
 * Write it to the result bin file

From the code commentary(describes how we merge chunks into one file):
```
Works only on sorted chuns!
Find minimal value among all chunk files(readers) at the same file pointer position
and write it to the result output file(thread-safely)

Like, image we have 2 chunks('^' means file pointer):

-- Iteration 0 --
[0, 1, 2]
 ^
[1, 1, 2]
 ^

-- Iteration 1 --
[0, 1, 2]
    ^       -> Wrote `0` to result file, iterate 1'st chunk from index 1, second from index 0
[1, 1, 2]
 ^

-- Iteration 3 --
[0, 1, 2]
    ^      -> Wrote `1` to result file, iterate 2'nd chunk from index 1, second from index 1
[1, 1, 2]
    ^

On each iteration we load to memory only values that are on the current chunk file pointer position, and then
comapre them to each other.
```

### Possible improvements
* Add new `Ozzy::ThreadPool` class instead of `std::vector<std::thread>`, with `std::queue` in it, which stores the requests that currently cannot be
processed. When one of the thread workers is free assign this request to him. Now we just drop the connection.
* Add symmetric encryption, transfer key using `Diffie-Hellman` algorithm
* Use asynchronous sockets
* Support architecture mismatch between client and server

