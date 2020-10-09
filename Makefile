.PHONY: all clean

all: frame_diff

frame_diff: src/frame_diff.c src/image_io.c src/median_filter.c
	gcc $^ -o bin/$@

clean:
	-rm -f bin/*