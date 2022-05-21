CC = gcc
CFLAGS = -O3 -Wall -Wextra -Wformat-security -Werror -Wpedantic -march=x86-64 -fstack-protector-all -Wstack-protector -pie -fPIE -D_FORTIFY_SOURCE=2 -fstack-clash-protection -g -std=c2x -Ldeps/libsodium-stable/src/libsodium/.libs/
SQLITE_FLAGS = -O3 -Wextra -Wformat-security -Wpedantic -march=x86-64 -fstack-protector-all -Wstack-protector -pie -fPIE -D_FORTIFY_SOURCE=2 -fstack-clash-protection -g -std=c2x
INC = -Iinc/ -Ideps/libsodium-stable/src/libsodium/include -Ideps/sqlite/sqlite-amalgamation-3380500
LIBS = -Bstatic -l:libsodium.a
OBJ_DIR = bin/obj/
EXEC_DIR = bin/exec/

libsodium:
	mkdir -p deps/libsodium-stable
	-rm -rf deps/libsodium-stable
	-rm -f deps/libsodium.tar.gz
	curl https://download.libsodium.org/libsodium/releases/LATEST.tar.gz -o deps/libsodium.tar.gz
	tar -xzf deps/libsodium.tar.gz -C deps/
	rm -f deps/libsodium.tar.gz
	cd deps/libsodium-stable; \
	./configure; \
	make && make check

sqlite:
	mkdir -p deps/sqlite
	-rm -rf deps/sqlite
	-rm -f deps/sqlite.zip
	curl https://sqlite.org/2022/sqlite-amalgamation-3380500.zip -o deps/sqlite.zip
	unzip deps/sqlite.zip -d deps/sqlite/
	rm -f deps/sqlite.zip

sqlite.o: deps/sqlite/sqlite-amalgamation-3380500/sqlite3.c deps/sqlite/sqlite-amalgamation-3380500/sqlite3.h
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(SQLITE_FLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

kv.o: src/kv.c inc/kv.h inc/sum_types.h inc/slice.h
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

test.o: src/test.c
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

passmngr.o: src/passmngr.c inc/passmngr.h
	mkdir -p $(OBJ_DIR)
	$(CC) -c $(CFLAGS) $(INC) $(LDFLAGS) $< $(LIBS) -o $(OBJ_DIR)$@

test: test.o kv.o sqlite.o
	mkdir -p $(EXEC_DIR)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@

passmngr: passmngr.o kv.o sqlite.o
	mkdir -p $(EXEC_DIR)
	$(CC) $(CFLAGS) $(INC) $(LDFLAGS) $(addprefix $(OBJ_DIR),$^) $(LIBS) -o $(EXEC_DIR)$@
