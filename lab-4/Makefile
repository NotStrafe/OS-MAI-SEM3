# Компилятор и флаги
CC      = gcc
CFLAGS  = -Wall -O2 -fPIC
LDFLAGS = -shared
RPATH   = -Wl,-rpath,.

# Цель по умолчанию: сборка обоих исполняемых файлов
all: prog_linked prog_dlopen

# Сборка разделяемой библиотеки libderivative.so
libderivative.so: derivative.o
	$(CC) $(LDFLAGS) -o libderivative.so derivative.o -lm

derivative.o: derivative.c derivative.h
	$(CC) $(CFLAGS) -c derivative.c

# Сборка разделяемой библиотеки libpicalc.so
libpicalc.so: picalc.o
	$(CC) $(LDFLAGS) -o libpicalc.so picalc.o

picalc.o: picalc.c picalc.h
	$(CC) $(CFLAGS) -c picalc.c

# Приложение с компоновкой библиотек (статическая линковка с динамическими библиотеками)
prog_linked: main_linked.c libderivative.so libpicalc.so
	$(CC) -Wall -o prog_linked main_linked.c -L. -lderivative -lpicalc -lm $(RPATH)

# Приложение с динамической загрузкой библиотек (через dlopen/dlsym)
prog_dlopen: main_dlopen.c libderivative.so libpicalc.so
	$(CC) -Wall -o prog_dlopen main_dlopen.c -ldl $(RPATH)

# Очистка сгенерированных файлов
clean:
	rm -f *.o *.so prog_linked prog_dlopen
