# strip-obu-dv

Simple program to parse a file containing OBUs for Dolby Vision (DV) OBUs. If an output is provided, the OBUs with DV metadata are stripped, thereby removing any DV data from the file. This process does not use transcoding at all, so the process is rather quick. A 10GB OBU file can be stripped in a matter of a few minutes.

## Why?

~~As of the creation of this program, ffmpeg does not have this program's features, despite being able to produce DV AV1 content through encoding (AV1 content is comprised of OBUs).~~ Recent versions of ffmpeg have this program's features. View the [`dovi_rpu` bitstream filter](https://ffmpeg.org/ffmpeg-bitstream-filters.html#dovi_005frpu) for more information. As many HW decoders are simply not capable of decoding AV1 DV streams, I wanted a tool to fix this problem without any re-encoding taking place.

## Integration with ffmpeg
This project **only parses OBU files**. Therefore, using it on an MP4/MKV file **will not work**. To strip DV content from these file types, you will need to demux with ffmpeg, then strip the OBU, then remux back into an MP4/MKV container. The [strip-with-ffmpeg.sh](scripts/strip-with-ffmpeg.sh) script does precisely that without any re-encoding taking place.

## Usage
To strip an OBU file from DV content:
```bash
$ strip-obu-dv -i test-data/dv.obu -o test-data/dv-removed.obu
input:test-data/dv.obu
input size:2836781
chunk size:800000
output:test-data/dv-removed.obu
total OBU read:434
total DV OBU read:130
total processing time:0 seconds
```

To parse without producing an output:
```bash
$ strip-obu-dv -i test-data/no-dv.obu
input:test-data/no-dv.obu
input size:702686
chunk size:800000
output:(null)
total OBU read:304
total DV OBU read:0
total processing time:0 seconds
```

OBUs do not have a standardized maximum size. While this project has tested a variety of OBU files, it is possible to fail to parse a file if the chunk size is too small. Smaller chunks will perform faster than larger ones, so the default behavior leans toward smaller chunk sizes. If you encounter an OBU file that the default chunk size fails at, create an issue. To allocate a larger chunk size capable of parsing larger-than-expected OBUs, set a larger chunk size:
```bash
$ ./build/strip-obu-dv -i test-data/no-dv.obu -c 5
input:test-data/no-dv.obu
input size:702686
chunk size:5
output:(null)
0.00%, ETA: 0, 0 DV OBU

error obp_get_next_obu
Invalid OBU size: larger than remaining buffer.
remaining buffer size:3
OBU size:15
try increasing chunk size

$ strip-obu-dv -i test-data/no-dv.obu -c 50000000
input:test-data/no-dv.obu
input size:702686
chunk size:50000000
output:(null)
total OBU read:304
total DV OBU read:0
total processing time:0 seconds
### SUCCESS
```

## Building
Building is done with cmake:
```bash
cmake -B build
cmake --build build
```
Alternatively, use the [build script](scripts/build.sh): `./scripts/build.sh`

Built and tested on:
- x86_64-pc-msys
- x86_64-pc-linux-gnu
- aarch64-unknown-linux-gnu
- aarch64-apple-darwin
