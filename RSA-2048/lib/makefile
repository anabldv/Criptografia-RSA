TARGET = rsa.dll
CFLAGS = -Wall -Wextra -O2 -fPIC -DPLATFORM_WINDOWS
LDFLAGS = -shared -lpsapi
SOURCES = FunRSA.c  

OBJS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	del *.o *.dll

.PHONY: all clean