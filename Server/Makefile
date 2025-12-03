
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -pthread -Wno-unused-parameter -Wno-unused-result 
TARGET = Server
SOURCES = main.cpp server.cpp threadpool.cpp
OBJS = $(SOURCES:.cpp=.o)
HEADERS = server.h threadpool.h

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: CXXFLAGS += -g -DDEBUG
debug: $(TARGET)

release: CXXFLAGS += -O3 -DNDEBUG
release: clean $(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/

run: $(TARGET)
	./$(TARGET)

info:
	@echo "Source files: $(SOURCES)"
	@echo "Header files: $(HEADERS)"
	@echo "Object files: $(OBJS)"

.PHONY: clean install debug release run info
# Установка
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
SYSTEMD_DIR = /etc/systemd/system

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/
	install -d $(SYSTEMD_DIR)
	install -m 644 Server.service $(SYSTEMD_DIR)/
	systemctl daemon-reload
	@echo "Service installed. Enable with: systemctl enable Server"

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(SYSTEMD_DIR)/Server.service
	systemctl daemon-reload
	@echo "Service uninstalled"

# Управление сервисом
start:
	systemctl start Server

stop:
	systemctl stop Server

status:
	systemctl status Server

restart:
	systemctl restart Server

# Просмотр логов
logs:
	journalctl -u Server -f

.PHONY: clean install uninstall debug release start stop status restart logs
# Создание пакета
PACKAGE_DIR = Server-pkg
PACKAGE_NAME = Server_1.0-1_amd64.deb

package: $(TARGET)
	mkdir -p $(PACKAGE_DIR)/DEBIAN
	mkdir -p $(PACKAGE_DIR)/usr/local/bin
	mkdir -p $(PACKAGE_DIR)/lib/systemd/system
	mkdir -p $(PACKAGE_DIR)/usr/share/doc/Server
	cp $(TARGET) $(PACKAGE_DIR)/usr/local/bin/
	cp Server.service $(PACKAGE_DIR)/lib/systemd/system/Server.service
	cp packaging/control $(PACKAGE_DIR)/DEBIAN/ 2>/dev/null || true
	# Если нет отдельных файлов, создаем их тут
	echo "#!/bin/bash\nset -e\nsystemctl daemon-reload\nsystemctl enable Server.service\necho 'Server installed'" > $(PACKAGE_DIR)/DEBIAN/postinst
	echo "#!/bin/bash\nset -e\nsystemctl stop Server.service 2>/dev/null || true\nsystemctl disable Server.service 2>/dev/null || true" > $(PACKAGE_DIR)/DEBIAN/prerm
	chmod 755 $(PACKAGE_DIR)/DEBIAN/postinst
	chmod 755 $(PACKAGE_DIR)/DEBIAN/prerm
	echo "MIT License" > $(PACKAGE_DIR)/usr/share/doc/Server/copyright
	dpkg-deb --build $(PACKAGE_DIR) $(PACKAGE_NAME)
	@echo "Package created: $(PACKAGE_NAME)"

clean-package:
	rm -rf $(PACKAGE_DIR)
	rm -f *.deb

.PHONY: package clean-package
