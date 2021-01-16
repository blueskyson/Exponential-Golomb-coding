all: encode decode

encode: exp-golomb.c
	gcc exp-golomb.c -o encode

decode: encode
	ln -s encode decode

test: sample_text.txt encode decode
	@echo "Encode sample_text.txt as text.encode use order-4 exp-golomb code..."
	./encode sample_text.txt text.encode 4
	@echo "Decode text.encode as text.decode..."
	./decode text.encode text.decode 4
	@echo "check difference between sample_text.txt and text.decode..."
	diff sample_text.txt text.decode
	rm text.encode text.decode

clean:
	rm encode decode
