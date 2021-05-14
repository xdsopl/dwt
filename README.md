### Playing with lossy image compression based on the discrete wavelet transformation

Quick start:

Encode [smpte.ppm](smpte.ppm) [PNM](https://en.wikipedia.org/wiki/Netpbm) picture file to ```encoded.dwt```:

```
./encode smpte.ppm encoded.dwt
```

Decode ```encoded.dwt``` file to ```decoded.ppm``` picture file:

```
./decode encoded.dwt decoded.ppm
```

Watch ```decoded.ppm``` picture file in [feh](https://feh.finalrewind.org/):

```
feh decoded.ppm
```

### Adjusting quantization

Use quantization values of seven for luminance (Y'), six and seven for chrominance (Cb and Cr) instead of the default ```7 5 5``` values:

```
./encode smpte.ppm encoded.dwt 7 6 5
```

### Use different wavelet

Use the [Haar wavelet](https://en.wikipedia.org/wiki/Haar_wavelet) instead of the default ```1``` [CDF](https://en.wikipedia.org/wiki/Cohen%E2%80%93Daubechies%E2%80%93Feauveau_wavelet) 9/7 wavelet:

```
./encode smpte.ppm encoded.dwt 7 5 5 0
```

### Limited storage capacity

Use up to ```65536``` bits of space and discard quality bits, if necessary, to stay below ```65536``` bits:

```
./encode smpte.ppm encoded.dwt 7 5 5 1 65536
```

