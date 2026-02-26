# OPer
OPer - Error Monitor and Recovery tool

Сборка:
```bash
mkdir build && cd build
cmake ..
make
```

Установка:
```bash
# Установите nlohmann-json (например, apt-get install nlohmann-json3-dev)
mkdir build && cd build
cmake ..
make
sudo make install   # установит бинарник в /usr/local/bin/oper и скопирует конфигурацию/сервис

# Создайте директорию для алгоритмов (если не создалась автоматически)
sudo mkdir -p /etc/oper/algorithms

# Скопируйте свои JSON-алгоритмы
sudo cp ../config/example.json /etc/oper/algorithms/

# Перезагрузите systemd и запустите сервис
sudo systemctl daemon-reload
sudo systemctl enable oper.service
sudo systemctl start oper.service
```
Thanks for https://github.com/nlohmann/json
