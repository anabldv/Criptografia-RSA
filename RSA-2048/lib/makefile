TARGET = rsa.dll
EXE = rsa.exe
CFLAGS = -Wall -Wextra -O2 -fPIC -DPLATFORM_WINDOWS
LDFLAGS = -shared -lbcrypt
SOURCES = FunRSA.c

OBJS = $(SOURCES:.c=.o)

all: $(TARGET) $(EXE)

$(TARGET): $(OBJS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

# versao em linha de comando (main.c usa a biblioteca diretamente)
$(EXE): main.c FunRSA.c
	gcc -Wall -Wextra -O2 main.c FunRSA.c -o $@ -lbcrypt

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	del *.o *.dll *.exe

.PHONY: all clean
