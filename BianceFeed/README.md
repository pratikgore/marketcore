Dependcies

curl : sudo apt-get install libcurl4-openssl-dev 
    :  sudo apt-get install libssl-dev
    :  sudo apt-get install libssl-dev libboost-dev libboost-system-dev
lohmann json : 

cd /home/pgore/workspace/marketcore/BianceFeed && g++ -std=c++17 -O0 -g clsWSBNConnect.cpp -o ws_bn_exe -pthread -lboost_system -lssl -lcrypto

build thorugh cmake : 
cmake -S . -B build
cmake --build build --target biance_adapt_exe -j