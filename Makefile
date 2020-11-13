CC = gcc
CFLAGS = -Wall -Wpedantic -g -std=c18
INC = -Isrc/archive/ -Isrc/vault/ -Isrc/result/ -Isrc/common/ -Isrc/hashmap
STATIC_LIB_DIR = bin/static/
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/

fasthash.o: src/hashmap/fasthash.c src/hashmap/fasthash.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

hashmap.o: src/hashmap/hashmap.c src/hashmap/hashmap.h fasthash.o
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive.o: src/archive/archive.c src/archive/archive.h hashmap.o
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test.o: test/archive_test.c 
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

archive_test: archive_test.o archive.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) -o $(EXEC_DIR)$@

vault.o: src/vault/vault.c src/vault/vault.h archive.o
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

vault_test.o: test/vault_test.c
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< -o $(OBJ_DIR)$@

vault_test: vault_test.o vault.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) -o $(EXEC_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(EXEC_DIR)*
