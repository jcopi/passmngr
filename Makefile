CC = gcc
CFLAGS = -Wall -Wpedantic -g
INC = -Isrc/archive/ -Isrc/vault/
STATIC_LIB_DIR = bin/static/
OBJ_DIR = bin/obj/
BIN_DIR = bin/
LDFLAGS = 

archive.o: src/archive/archive.c src/archive/archive.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test.o: test/archive_test.c 
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test: archive_test.o archive.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) -o $(BIN_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(BIN_DIR)*
