CC = gcc
CFLAGS = -Wall -Wpedantic -g -std=c18 -L/usr/local/lib/
INC = -Isrc/archive/ -Isrc/vault/ -Isrc/result/ -Isrc/common/ -Isrc/kv_store -I/usr/local/sodium 
LIBS = -lsodium
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/

kv_store.o: src/kv_store/kv_store.c src/kv_store/kv_store.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_store_test.o: test/kv_store_test.c
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_store_test: kv_store_test.o kv_store.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

archive.o: src/archive/archive.c src/archive/archive.h hashmap.o
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

archive_test.o: test/archive_test.c 
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

archive_test: archive_test.o archive.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

vault.o: src/vault/vault.c src/vault/vault.h archive.o
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

vault_test.o: test/vault_test.c
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

vault_test: vault_test.o vault.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(EXEC_DIR)*
