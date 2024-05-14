### Playing with lossless image compression based on the discrete wavelet transformation

Quick start:

Encode [smpte.pnm](smpte.pnm) [PNM](https://en.wikipedia.org/wiki/Netpbm) picture file to ```encoded.dwt```:

```
./encode smpte.pnm encoded.dwt
```

Decode ```encoded.dwt``` file to ```decoded.pnm``` picture file:

```
./decode encoded.dwt decoded.pnm
```

Watch ```decoded.pnm``` picture file in [feh](https://feh.finalrewind.org/):

```
feh decoded.pnm
```

### Limited storage capacity

Use up to ```65536``` bytes of space instead of the default ```0``` (no limit) and discard quality bytes, if necessary, to stay below ```65536``` bytes:

```
./encode smpte.pnm encoded.dwt 65536
```

### Use different wavelet

Use the reversible integer [Haar wavelet](https://en.wikipedia.org/wiki/Haar_wavelet) instead of the default ```0``` reversible integer [CDF](https://en.wikipedia.org/wiki/Cohen%E2%80%93Daubechies%E2%80%93Feauveau_wavelet) 5/3 wavelet:

```
./encode smpte.pnm encoded.dwt 0 1
```

### Reading

* Run-length encodings  
by Solomon W. Golomb - 1966
* Image coding using wavelet transform  
by M. Antonini, M. Barlaud, P. Mathieu and I. Daubechies - 1992
* Factoring wavelet transforms into lifting steps  
by Ingrid Daubechies and Wim Sweldens - 1996

