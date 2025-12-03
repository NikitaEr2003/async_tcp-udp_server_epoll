# Async TCP/UDP Server

Асинхронный сервер на C++ для обработки TCP и UDP подключений с использованием epoll.

## Возможности
- Асинхронная обработка TCP и UDP соединений
- Мультиплексирование на базе epoll
- Команды: /time, /stats, /shutdown
- Зеркалирование сообщений клиентам
- По умолчанию сервер работает на порту 8888, но вы можете указать другой порт и количество потоков через аргументы при запуске.

## Быстрая установка из пакета
```bash
sudo dpkg -i Server_1.0-1_amd64.deb
sudo systemctl start Server
sudo systemctl status Server

##Установка из исходников
```bash
make
sudo make install
sudo systemctl enable Server
sudo systemctl start Server
sudo systemctl status Server

##Управление
```bash
sudo systemctl start Server    # Запуск
sudo systemctl stop Server     # Остановка
sudo systemctl restart Server  # Перезапуск
sudo systemctl list-units --type=service # Список всех systemd-служб
sudo systemctl status Server   # Статус
sudo journalctl -u Server -f   # Логи

##Структура проекта 
Server/
├── CMakeLists.txt
├── Makefile
├── Server_1.0-1_amd64.deb
├── Server.service
├── main.cpp
├── server.cpp
├── server.h
├── threadpool.cpp
├── threadpool.h
README.md
