CC = gcc
CFLAGS = -Wall -Wpedantic -g
INC = -Isrc/archive/ -Isrc/vault/ -Isrc/result/
STATIC_LIB_DIR = bin/static/
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/

archive.o: src/archive/archive.c src/archive/archive.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test.o: test/archive_test.c 
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test: archive_test.o archive.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) -o $(EXEC_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(EXEC_DIR)*
