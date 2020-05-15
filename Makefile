PROJECT=drv8835_daemon
DAEMON_SOURCES=daemon.c
LIB_SOURCES=drv8835.c motor_server.c socket_server.c socket_client.c types.c common.c
TEST_THREAD_SRC=test_thread.c common.c
TEST_SOCKET_SRV_SRC=test_socket_srv.c common.c
TEST_SOCKET_CLT_SRC=test_socket_clt.c common.c
TEST_DRV8835_MOTOR_SERVER_SRC=test_drv8835_motor_server.c
TEST_DRV8835_SOCKET_SERVER_SRC=test_drv8835_socket_server.c
TEST_DRV8835_SOCKET_CLIENT_SRC=test_drv8835_socket_client.c
TEST_SDL_CLIENT_SRC=test_sdl_client.c
SOURCES=$(DAEMON_SOURCES) $(LIB_SOURCES) $(TEST_THREAD_SRC) $(TEST_SOCKET_SRV_SRC) $(TEST_SOCKET_CLT_SRC) $(TEST_DRV8835_MOTOR_SERVER_SRC) $(TEST_DRV8835_SOCKET_SERVER_SRC) $(TEST_DRV8835_SOCKET_CLIENT_SRC) $(TEST_SDL_CLIENT_SRC)
INCPATHS=./ /usr/local/include/
LIBPATHS=./
LDFLAGS=-lwiringPi -lpthread
SDL_LDFLAGS=-lSDL2
CFLAGS=-c -Wall
CC=gcc
AR=ar
ARFLAGS=rcs

# Automatic generation of some important lists
DAEMON_OBJECTS=$(DAEMON_SOURCES:.c=.o)
LIB_OBJECTS=$(LIB_SOURCES:.c=.o)
TEST_THREAD_OBJS=$(TEST_THREAD_SRC:.c=.o)
TEST_SOCKET_SRV_OBJS=$(TEST_SOCKET_SRV_SRC:.c=.o)
TEST_SOCKET_CLT_OBJS=$(TEST_SOCKET_CLT_SRC:.c=.o)
TEST_DRV8835_MOTOR_SERVER_OBJS=$(TEST_DRV8835_MOTOR_SERVER_SRC:.c=.o)
TEST_DRV8835_SOCKET_SERVER_OBJS=$(TEST_DRV8835_SOCKET_SERVER_SRC:.c=.o)
TEST_DRV8835_SOCKET_CLIENT_OBJS=$(TEST_DRV8835_SOCKET_CLIENT_SRC:.c=.o)
TEST_SDL_CLIENT_OBJS=$(TEST_SDL_CLIENT_SRC:.c=.o)
OBJECTS=$(DAEMON_OBJECTS) $(LIB_OBJECTS) $(TEST_THREAD_OBJS) $(TEST_SOCKET_SRV_OBJS) $(TEST_SOCKET_CLT_OBJS) $(TEST_DRV8835_MOTOR_SERVER_OBJS) $(TEST_DRV8835_SOCKET_SERVER_OBJS) $(TEST_DRV8835_SOCKET_CLIENT_OBJS) $(TEST_SDL_CLIENT_OBJS)
INCFLAGS=$(foreach TMP,$(INCPATHS),-I$(TMP)) 
LIBFLAGS=$(foreach TMP,$(LIBPATHS),-L$(TMP))

DAEMON_BINARY=$(PROJECT)
LIB_BIN=libdrv8835.a
TEST_THREAD_BINARY=drv8835_test_thread
TEST_SOCKET_SRV_BIN=drv8835_test_socket_srv
TEST_SOCKET_CLT_BIN=drv8835_test_socket_clt
TEST_DRV8835_MOTOR_SERVER_BIN=drv8835_test_motor_server
TEST_DRV8835_SOCKET_SERVER_BIN=drv8835_test_socket_server
TEST_DRV8835_SOCKET_CLIENT_BIN=drv8835_test_socket_client
TEST_SDL_CLIENT_BIN=drv8835_test_sdl_client
BINARY=$(DAEMON_BINARY) $(LIB_BIN) $(TEST_THREAD_BINARY) $(TEST_SOCKET_SRV_BIN) $(TEST_SOCKET_CLT_BIN) $(TEST_DRV8835_MOTOR_SERVER_BIN) $(TEST_DRV8835_SOCKET_SERVER_BIN) $(TEST_DRV8835_SOCKET_CLIENT_BIN) $(TEST_SDL_CLIENT_BIN)
all: $(SOURCES) $(BINARY)

$(DAEMON_BINARY): $(DAEMON_OBJECTS)
	$(CC) $(LIBFLAGS) $(DAEMON_OBJECTS) $(LDFLAGS) -o $@
$(LIB_BIN): $(LIB_OBJECTS)
	$(AR) $(ARFLAGS) $@ $(LIB_OBJECTS)
$(TEST_THREAD_BINARY): $(TEST_THREAD_OBJS)
	$(CC) $(LIBFLAGS) $(TEST_THREAD_OBJS) $(LDFLAGS) -o $@
$(TEST_SOCKET_SRV_BIN): $(TEST_SOCKET_SRV_OBJS)
	$(CC) $(LIBFLAGS) $(TEST_SOCKET_SRV_OBJS) $(LDFLAGS) -o $@
$(TEST_SOCKET_CLT_BIN): $(TEST_SOCKET_CLT_OBJS)
	$(CC) $(LIBFLAGS) $(TEST_SOCKET_CLT_OBJS) $(LDFLAGS) -o $@
$(TEST_DRV8835_MOTOR_SERVER_BIN): $(TEST_DRV8835_MOTOR_SERVER_OBJS) $(LIB_BIN)
	$(CC) $(LIBFLAGS) $(TEST_DRV8835_MOTOR_SERVER_OBJS) $(LDFLAGS) -ldrv8835 -o $@
$(TEST_DRV8835_SOCKET_SERVER_BIN): $(TEST_DRV8835_SOCKET_SERVER_OBJS) $(LIB_BIN)
	$(CC) $(LIBFLAGS) $(TEST_DRV8835_SOCKET_SERVER_OBJS) $(LDFLAGS) -ldrv8835 -o $@
$(TEST_DRV8835_SOCKET_CLIENT_BIN): $(TEST_DRV8835_SOCKET_CLIENT_OBJS) $(LIB_BIN)
	$(CC) $(LIBFLAGS) $(TEST_DRV8835_SOCKET_CLIENT_OBJS) $(LDFLAGS) -ldrv8835 -o $@
$(TEST_SDL_CLIENT_BIN): $(TEST_SDL_CLIENT_OBJS) $(LIB_BIN)
	$(CC) $(LIBFLAGS) $(TEST_SDL_CLIENT_OBJS) $(LDFLAGS) $(SDL_LDFLAGS) -ldrv8835 -o $@
.c.o:
	$(CC) $(INCFLAGS) $(CFLAGS) -fPIC $< -o $@

distclean: clean
	rm -f $(BINARY)

clean:
	rm -f $(OBJECTS)
