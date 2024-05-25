# Compile
The program could only be compiled with clang.
To install clang, you could refer [this](https://apt.llvm.org/).

```
apt-get install libboost-all-dev
mkdir build && cmake ..
```
# Chat
```
./server port 
./client client_id_ingeter host port
```

# Message Format
``` 
MsgLen(size_t) + MsgType(size_t) + MsgSrc(size_t) + MsgSeq(size_t) + MsgContent(binary)

```

## ACK Message
```
MsgLen(36) + ACK + MsgSrc(size_t) + MsgSeq(size_t) + target_seq_to_ack 

```

# ASAN
```
cmake .. -DENABLE_ASAN=0
```
The program is checked with asan.