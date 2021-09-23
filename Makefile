CC = gcc
CFLAGS = -Wall -Wextra -Wformat-security -Werror -Wpedantic -march=x86-64 -fstack-protector-all -Wstack-protector -pie -fPIE -D_FORTIFY_SOURCE=2 -fstack-clash-protection -g -std=c18 -Ldeps/libsodium/src/.libs/
INC = -Isrc/archive/ -Isrc/vault/ -Isrc/result/ -Isrc/ -Isrc/common/ -Isrc/kv_store -Ideps/libsodium/include 
LIBS = -Bstatic -l:libsodium.a
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/
	
slice.o: src/kv_store/slice.c src/kv_store/slice.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

result.o: src/kv_store/result.c src/kv_store/result.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv.o: src/kv_store/kv.c src/kv_store/kv.h src/kv_store/kv_types.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_test.o: test/kv_test.c
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_store_bench.o: test/kv_store_bench.c src/wordlist.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv_test: kv_test.o kv.o slice.o result.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

kv_store_bench: kv_store_bench.o kv_store.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

passmngr.o: src/passmngr.c src/passmngr.h src/wordlist.h
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

passmngr: passmngr.o kv_store.o
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

.phony: clean
clean:
	-rm $(OBJ_DIR)*.o
	-rm $(EXEC_DIR)*
