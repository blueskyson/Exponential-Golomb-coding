# Exponential-Golomb-coding

### Compile

```
$ make
gcc exp-golomb.c -o encode
ln -s encode decode
```

### Usage

```
# Usage
encode [input file] [output file] [order-k]
decode [input file] [output file] [order-k]

# Example
./encode sample_text.txt text.encode 4
./decode text.encode text.decode 4
```

### Test

```
$ make test
Encode sample_text.txt as text.encode use order-4 exp-golomb code...
./encode sample_text.txt text.encode 4
Decode text.encode as text.decode...
./decode text.encode text.decode 4
check difference between sample_text.txt and text.decode...
diff sample_text.txt text.decode
rm text.encode text.decode
```