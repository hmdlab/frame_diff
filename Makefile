.PHONY: all clean

all: bin/frame_diff

bin/frame_diff: src/frame_diff.c src/image_io.c src/median_filter.c
	gcc $^ -o $@

clean:
	-rm -f bin/*
