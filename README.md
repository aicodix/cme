
### Cauchy Mersenne Prime Field Erasure Coding

Quick start:

Create file ```input.dat``` with ```128KiB``` of random data:

```
dd if=/dev/urandom of=input.dat bs=512 count=256
```

Split file ```input.dat``` 123 times and encode a thousand redundant chunk files, each about a kilobyte in size:

```
./encode input.dat 123 chunk{000..999}.cme
```

Output should be ```CME(1000, 124)```, which means we only need any 124 chunks of the 1000 encoded.

Randomly delete (erase) 876 chunk files to only keep 124:

```
ls chunk*.cme | sort -R | head -n 876 | xargs rm
```

Decode ```output.dat``` from remaining chunk files:

```
./decode output.dat chunk*.cme
```

Compare original ```input.dat``` with ```output.dat```:

```
diff -q -s input.dat output.dat
```

