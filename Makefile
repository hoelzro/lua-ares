CFLAGS += -fPIC -I/usr/include/lua5.1/ -Wall -Werror
LIBS += -lcares

ares.so: ares.o
	$(CC) $(CFLAGS) -o $@ -shared $^ $(LIBS)
