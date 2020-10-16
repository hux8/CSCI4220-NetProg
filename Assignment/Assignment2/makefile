CC=clang  #compiler
TARGET=word_guess.out #target file name

all:
	$(CC) wg2.c dict.c server.c -L./unp -Wall  -o  $(TARGET)
clean:
	rm $(TARGET)