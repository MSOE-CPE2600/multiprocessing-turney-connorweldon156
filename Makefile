CC=gcc
CFLAGS=-c -Wall -g
LDFLAGS=-ljpeg
SOURCES= mandel.c jpegrw.c
MOVIE_SRC= mandelmovie.c
OBJECTS=$(SOURCES:.c=.o)
MOVIE_OBJ=$(MOVIE_SRC:.c=.o)

EXECUTABLE=mandel
MOVIE_EXEC=mandelmovie

all: $(EXECUTABLE) $(MOVIE_EXEC)

-include $(OBJECTS:.o=.d)
-include $(MOVIE_OBJ:.o=.d)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(MOVIE_EXEC): $(MOVIE_OBJ)
	$(CC) $(MOVIE_OBJ) -lm -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@
	$(CC) -MM $< > $*.d

clean:
	rm -rf $(OBJECTS) $(MOVIE_OBJ) $(EXECUTABLE) $(MOVIE_EXEC) *.d