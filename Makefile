CC = gcc
CFLAGS = -Wall -Wpedantic -g -std=c18 -Ldeps/libsodium/src/.libs/
INC = -Isrc/archive/ -Isrc/vault/ -Isrc/result/ -Isrc/common/ -Isrc/kv_store -Ideps/libsodium/src/libsodium/include 
LIBS = -Bstatic -l:libsodium.a
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/

libsodium.a: 
	cd deps/libsodium; \
	./configure; \
	make && make check
	
kv_store.o: src/kv_store/kv_store.c src/kv_store/kv_store.h libsodium.a
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_store_test.o: test/kv_store_test.c
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_store_test: kv_store_test.o kv_store.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

passmngr.o: src/passmngr.c src/passmngr.h src/wordlist.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

passmngr: passmngr.o kv_store.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(EXEC_DIR)*
