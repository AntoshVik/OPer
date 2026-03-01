# OPer — Error Monitor and Recovery tool

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**OPer** — это мощный и гибкий инструмент для мониторинга системных логов (ядра) и автоматического выполнения сценариев восстановления при обнаружении заданных ошибок. Программа написана на C++17, поддерживает асинхронное выполнение действий, таймауты, вложенные сценарии и динамическую перезагрузку конфигурации без остановки.

---

## 🌟 Возможности

- Мониторинг `/dev/kmsg` или любого лог-файла в реальном времени.
- Гибкие сценарии восстановления в формате JSON.
- Поддержка регулярных выражений для поиска событий.
- Асинхронное выполнение цепочек действий (не блокирует мониторинг).
- Таймауты для каждой команды и действия при превышении (`on_timeout`).
- Возможность игнорировать ошибки отдельных шагов (`ignore_failure`).
- Динамическая перезагрузка конфигурации при изменении файлов алгоритмов.
- Централизованный конфигурационный файл `/etc/oper/oper.conf`.
- Интеграция с systemd.

---

## 🚀 Установка

### Требования

- Компилятор с поддержкой C++17 (GCC 8+, Clang 7+).
- CMake 3.14 или новее.
- Библиотека `nlohmann/json` (скачивается автоматически при сборке).

### Сборка из исходников

```bash
git clone https://github.com/AntoshVik/OPer.git
cd OPer
mkdir build && cd build
cmake ..
make
sudo make install
```

Это установит:
- Исполняемый файл `/usr/local/bin/oper`
- Пример конфигурации в `/etc/oper/oper.conf`
- Пример алгоритма в `/etc/oper/algorithms/example.json`
- systemd-сервис `/etc/systemd/system/oper.service`

### Запуск как сервис

```bash
sudo systemctl daemon-reload
sudo systemctl enable oper.service
sudo systemctl start oper.service
sudo journalctl -u oper -f   # просмотр логов
```

---

## ⚙️ Конфигурация

### Основной файл `/etc/oper/oper.conf`

```json
{
    "log_source": "/dev/kmsg",
    "reload_interval": 5,
    "algorithms_dir": "/etc/oper/algorithms",
    "default_cooldown": 60,
    "max_parallel_tasks": 5
}
```

| Поле | Описание |
|------|----------|
| `log_source` | Источник логов: `/dev/kmsg` или путь к файлу (например, `/var/log/kern.log`) |
| `reload_interval` | Интервал (в секундах) проверки изменений в директории алгоритмов |
| `algorithms_dir` | Директория с JSON-файлами алгоритмов |
| `default_cooldown` | Значение cooldown по умолчанию (если не указано в алгоритме) |
| `max_parallel_tasks` | Максимальное количество одновременно выполняемых цепочек (пока не реализовано) |

### Формат алгоритма (JSON)

Каждый файл в `algorithms_dir` должен иметь расширение `.json` и содержать описание одного алгоритма:

```json
{
    "pattern": "Serverclose failed 4 times, giving up",
    "cooldown": 60,
    "actions": [
        {
            "command": "systemctl stop myapp.service",
            "timeout": 30,
            "ignore_failure": false,
            "on_timeout": {
                "command": "systemctl kill myapp.service",
                "timeout": 5,
                "ignore_failure": true
            }
        },
        {
            "command": "umount /mnt/data",
            "timeout": 10,
            "ignore_failure": false
        },
        {
            "command": "systemctl start myapp.service",
            "timeout": 30,
            "ignore_failure": false
        }
    ]
}
```

- `pattern` — регулярное выражение для поиска в строке лога.
- `cooldown` — минимальное время (в секундах) между срабатываниями одного алгоритма.
- `actions` — массив действий, выполняемых последовательно.
- Каждое действие содержит:
  - `command` — команда для выполнения (передаётся в `/bin/sh -c`).
  - `timeout` — максимальное время выполнения в секундах.
  - `ignore_failure` — если `true`, ошибка команды не прерывает цепочку.
  - `on_timeout` — (опционально) действие, выполняемое при превышении таймаута. Может быть вложенным.

---

## 📖 Использование

После запуска OPer начинает мониторинг указанного источника логов. При обнаружении строки, соответствующей `pattern` какого-либо алгоритма, проверяется cooldown. Если с последнего срабатывания прошло достаточно времени, создаётся копия алгоритма и запускается асинхронное выполнение цепочки действий.

### Просмотр статуса

```bash
systemctl status oper
journalctl -u oper -f
```

### Добавление собственных алгоритмов

Просто поместите JSON-файл в `/etc/oper/algorithms/`. OPer автоматически подхватит его в течение `reload_interval` секунд (или сразу, если изменить файл после запуска).

---

## 🧪 Пример

Допустим, в логах ядра появляется сообщение `usb 1-1: reset high-speed USB device number 2 using ehci_hcd`. Вы хотите перезапустить службу, связанную с USB-устройством. Создайте файл `/etc/oper/algorithms/usb_reset.json`:

```json
{
    "pattern": "reset high-speed USB device",
    "cooldown": 30,
    "actions": [
        {
            "command": "systemctl restart usb-service",
            "timeout": 20,
            "ignore_failure": false
        }
    ]
}
```

При каждом появлении такой строки (но не чаще раза в 30 секунд) будет выполняться перезапуск службы.

---

## 📂 Структура проекта

```
oper/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── ConfigLoader.cpp
│   ├── LogMonitor.cpp
│   ├── ActionExecutor.cpp
│   ├── Settings.cpp
│   ├── Algorithm.h
│   ├── ConfigLoader.h
│   ├── LogMonitor.h
│   ├── ActionExecutor.h
│   └── Settings.h
├── config/
│   ├── oper.conf
│   └── algorithms/
│       └── example.json
└── oper.service
```

---

## 📄 Лицензия

Проект распространяется под лицензией MIT. Подробнее см. файл [LICENSE](LICENSE).

---

## 🤝 Авторы

Разработано AntoshVik.

Если у вас есть вопросы или предложения, создавайте issue или pull request.

---

# English version

## OPer — Error Monitor and Recovery tool

**OPer** is a powerful and flexible tool for monitoring system logs (kernel) and automatically executing recovery scenarios when specified errors are detected. Written in C++17, it supports asynchronous execution, timeouts, nested actions, and dynamic configuration reload without stopping.

### Features

- Real‑time monitoring of `/dev/kmsg` or any log file.
- Flexible recovery scenarios in JSON format.
- Regular expression matching for events.
- Asynchronous action chains (non‑blocking monitoring).
- Timeouts per command and fallback actions (`on_timeout`).
- Option to ignore failures (`ignore_failure`).
- Dynamic configuration reload when algorithm files change.
- Central config file `/etc/oper/oper.conf`.
- systemd integration.

### Installation

#### Requirements

- C++17 compiler (GCC 8+, Clang 7+).
- CMake 3.14+.
- `nlohmann/json` (downloaded automatically during build).

#### Build from source

```bash
git clone https://github.com/AntoshVik/OPer.git
cd OPer
mkdir build && cd build
cmake ..
make
sudo make install
```

This installs:
- Binary `/usr/local/bin/oper`
- Example config `/etc/oper/oper.conf`
- Example algorithm `/etc/oper/algorithms/example.json`
- systemd service `/etc/systemd/system/oper.service`

#### Run as a service

```bash
sudo systemctl daemon-reload
sudo systemctl enable oper.service
sudo systemctl start oper.service
sudo journalctl -u oper -f   # view logs
```

### Configuration

#### Main file `/etc/oper/oper.conf`

```json
{
    "log_source": "/dev/kmsg",
    "reload_interval": 5,
    "algorithms_dir": "/etc/oper/algorithms",
    "default_cooldown": 60,
    "max_parallel_tasks": 5
}
```

| Field | Description |
|-------|-------------|
| `log_source` | Log source: `/dev/kmsg` or a file path (e.g., `/var/log/kern.log`) |
| `reload_interval` | Interval (seconds) to check for changes in `algorithms_dir` |
| `algorithms_dir` | Directory containing JSON algorithm files |
| `default_cooldown` | Default cooldown if not specified in an algorithm |
| `max_parallel_tasks` | Max concurrent action chains (not yet implemented) |

#### Algorithm format (JSON)

Each file in `algorithms_dir` must have `.json` extension and describe one algorithm:

```json
{
    "pattern": "Serverclose failed 4 times, giving up",
    "cooldown": 60,
    "actions": [
        {
            "command": "systemctl stop myapp.service",
            "timeout": 30,
            "ignore_failure": false,
            "on_timeout": {
                "command": "systemctl kill myapp.service",
                "timeout": 5,
                "ignore_failure": true
            }
        },
        {
            "command": "umount /mnt/data",
            "timeout": 10,
            "ignore_failure": false
        },
        {
            "command": "systemctl start myapp.service",
            "timeout": 30,
            "ignore_failure": false
        }
    ]
}
```

- `pattern` – regular expression to match in a log line.
- `cooldown` – minimum seconds between triggers of the same algorithm.
- `actions` – array of actions executed sequentially.
- Each action contains:
  - `command` – shell command (passed to `/bin/sh -c`).
  - `timeout` – maximum execution time in seconds.
  - `ignore_failure` – if `true`, command failure won't stop the chain.
  - `on_timeout` – (optional) action to execute on timeout; can be nested.

### Usage

Once started, OPer monitors the specified log source. When a line matches any algorithm's `pattern`, the cooldown is checked. If enough time has passed since the last trigger, a copy of the algorithm is created and the action chain is executed asynchronously.

#### Check status

```bash
systemctl status oper
journalctl -u oper -f
```

#### Add your own algorithms

Simply place a JSON file into `/etc/oper/algorithms/`. OPer will pick it up within `reload_interval` seconds (or immediately if you modify the file after startup).

### Example

Suppose kernel logs show `usb 1-1: reset high-speed USB device number 2 using ehci_hcd`. You want to restart a service related to the USB device. Create `/etc/oper/algorithms/usb_reset.json`:

```json
{
    "pattern": "reset high-speed USB device",
    "cooldown": 30,
    "actions": [
        {
            "command": "systemctl restart usb-service",
            "timeout": 20,
            "ignore_failure": false
        }
    ]
}
```

Every time such a line appears (but at most once per 30 seconds), the service will be restarted.

### Project structure

```
oper/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── ConfigLoader.cpp
│   ├── LogMonitor.cpp
│   ├── ActionExecutor.cpp
│   ├── Settings.cpp
│   ├── Algorithm.h
│   ├── ConfigLoader.h
│   ├── LogMonitor.h
│   ├── ActionExecutor.h
│   └── Settings.h
├── config/
│   ├── oper.conf
│   └── algorithms/
│       └── example.json
└── oper.service
```

### License

MIT. See [LICENSE](LICENSE) for details.

### Authors

Developed by AntoshVik.

Feel free to open issues or pull requests.
