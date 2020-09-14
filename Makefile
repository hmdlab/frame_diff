all: frame_diff clean

frame_diff: frame_diff.o image_io.o median_filter.o
	gcc frame_diff.o image_io.o median_filter.o -o bin/frame_diff

frame_diff.o:
	gcc -c src/frame_diff.c

image_io.o:
	gcc -c src/image_io.c src/image_io.h
	
median_filter.o:
	gcc -c src/median_filter.c src/median_filter.h

clean:
	-rm -f *.o
