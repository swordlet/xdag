# RandomX Algorithm in XDAG



## Requirements:

#### Install RandomX library

```bash
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DARCH=native ..
make
sudo make install
```

#### Enable huge pages

Temporary (until next reboot) reserve huge pages

```bash
sudo sysctl -w vm.nr_hugepages=1280
```

Permanent huge pages reservation

```bash
sudo bash -c "echo vm.nr_hugepages=1280 >> /etc/sysctl.conf"
```



## Launch parameters (test net):

#### pool

```bash
./xdag -t -randomx -disable-refresh -f pool.config -threads 1
```

The pool will switch to randomx algorithm, when fork time frame ( defined by constant in rx_hash.h) coming , automatically. 

#### miner

##### before RandomX fork:

```bash
./xdag -t -m <MINING_THREAD_NUMBER>  <POOL_ADDRESS>:<POOL_PORT> -a <WALLET_ADDRESS>
```

##### after RandomX fork:

```bash
./xdag -t -randomx -m <MINING_THREAD_NUMBER> <POOL_ADDRESS>:<POOL_PORT> -a <WALLET_ADDRESS>
```

The miner can't change PoW algorithm automatically.   So miner must turn on switch '-randomx‘  after algorithm fork manually.

##### Windows x64 CPU RandowX Miner 

```bash
DaggerMiner -cpu  -p <POOL_ADDRESS>:<POOL_PORT> -t <MINING_THREAD_NUMBER> -a <WALLET_ADDRESS>
```

repository https://github.com/swordlet/DaggerRandomxMiner/tree/RandomX

release https://github.com/swordlet/DaggerRandomxMiner/releases/tag/v0.4.0

miner not support GPU yet.



## Algorithm Description:

#### algorithm seed and fork 

- selecting seed block that `seedBlockHeight % 4096 == 0 `
- using sha256 hash of seed block as new randomx seed
- changing the seed  at time frame   `(Time frame of seedBlockHeight) + 128  `
- pool algorithm fork at time frame   `(Time frame of RANDOMX_FORK_HEIGHT) + 128  `

#### mining

- every 64 Bytes mining task contains: 32 Bytes sha256 hash of mining block without nonce( i.e., without last field) and 32 Bytes randomx seed 
- randomx miner using  32 Bytes sha256 hash of mining block concatenate 32 Bytes nonce as input, searching nonce for minimal  randomx hash 

- miner send mined nonce back  to pool,  pool verify nonce with randomx hash and calculate share payment by difficulty of the randomx hash

####  hash of block

- using block's randomx hash to calculate its difficulty when block time is end with 0xffff, otherwise using block's Dsha256 hash
- using block's Dsha256 hash  as its hash value in any other circumstance



## Implementations:

