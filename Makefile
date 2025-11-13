CC = gcc
CFLAGS = -Wall -Wextra -std=c11
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)
TARGET = proyecto-4aa
SOURCE = proyecto-4aa.c
GLADE_FILE = proyecto-4aa.glade

all: $(TARGET)

$(TARGET): $(SOURCE) $(GLADE_FILE)
	$(CC) $(CFLAGS) $(GTK_CFLAGS) -rdynamic -o $(TARGET) $(SOURCE) $(GTK_LIBS)

clean:
	rm -f $(TARGET) *.o *.tex *.aux *.log *.pdf *.out

install-deps:
	@echo "Instalando dependencias..."
	@if command -v apt-get > /dev/null; then \
		sudo apt-get update && sudo apt-get install -y libgtk-3-dev pkg-config texlive-latex-base texlive-latex-extra texlive-fonts-recommended evince; \
	elif command -v yum > /dev/null; then \
		sudo yum install -y gtk3-devel pkgconfig texlive-latex texlive-latex-extra texlive-collection-latex evince; \
	elif command -v pacman > /dev/null; then \
		sudo pacman -S --noconfirm gtk3 texlive-most evince; \
	else \
		echo "Por favor instale manualmente: libgtk-3-dev, pkg-config, texlive, evince"; \
	fi

.PHONY: all clean install-deps


