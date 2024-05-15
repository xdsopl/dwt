### Discrete Wavelet Transform (DWT) Based Lossless Image Compression with Bitstream Truncation Support

Quick start:

Encode the [smpte.pnm](smpte.pnm) [PNM](https://en.wikipedia.org/wiki/Netpbm) picture file into ```encoded.dwt```:

```
./encode smpte.pnm encoded.dwt
```

Decode ```encoded.dwt``` back to the original picture file ```decoded.pnm```:

```
./decode encoded.dwt decoded.pnm
```

View the ```decoded.pnm``` picture file in [feh](https://feh.finalrewind.org/):

```
feh decoded.pnm
```

### Limited Storage Capacity: Bitstream Truncation

Allocate a maximum of ```65536``` bytes of space for compression. If necessary, the encoder will discard quality bytes to ensure the output stays below ```65536``` bytes:

```
./encode smpte.pnm encoded.dwt 65536
```

### Using a Different Wavelet

Optionally, utilize the [Haar wavelet](https://en.wikipedia.org/wiki/Haar_wavelet) instead of the default [CDF](https://en.wikipedia.org/wiki/Cohen%E2%80%93Daubechies%E2%80%93Feauveau_wavelet) 5/3 wavelet:

```
./encode smpte.pnm encoded.dwt 0 1
```

### References

* Run-length encodings  
by Solomon W. Golomb - 1966
* Image coding using wavelet transform  
by M. Antonini, M. Barlaud, P. Mathieu and I. Daubechies - 1992
* Factoring wavelet transforms into lifting steps  
by Ingrid Daubechies and Wim Sweldens - 1996

