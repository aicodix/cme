
### Cauchy Mersenne Prime Field Erasure Coding

Quick start:

Create file ```input.dat``` with ```128KiB``` of random data:

```
dd if=/dev/urandom of=input.dat bs=512 count=256
```

Encode file ```input.dat``` to a thousand chunk files, each of ```1024``` bytes in size:

```
./encode input.dat 1k chunk{000..999}.cme
```

Output should be ```CME(1000, 131)```, which means we only need any 131 chunks of the 1000 encoded.

Randomly delete (erase) 869 chunk files to only keep 131:

```
ls chunk*.cme | sort -R | head -n 869 | xargs rm
```

Decode ```output.dat``` from remaining chunk files:

```
./decode output.dat chunk*.cme
```

Compare original ```input.dat``` with ```output.dat```:

```
diff -q -s input.dat output.dat
```

