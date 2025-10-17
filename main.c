#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "obuparse/obuparse.h"

static size_t CHUNK_SIZE = (sizeof(uint8_t) * 500 * 1000);
#define ERROR_BUFFER_SIZE 128

void usage(char *progname) {
  printf("incorrect usage\n");
  printf("%s -i input.obu [options]\n", progname);
  printf("options:\n");
  printf("\t-c <num>\tset chunk size in bytes (default %ld)\n", CHUNK_SIZE);
  printf("\t-o <outfile>\toutput to a file without DV OBU\n");
  return;
}

int32_t ReadNextChunk(uint8_t *buf, const size_t bufSize,
                      const size_t chunkSize, FILE *fp,
                      const int64_t fileSize) {
  int32_t retval = 1;
  size_t freadSize = 0;
  size_t readSize = 0;

  if (fileSize < chunkSize) {
    readSize = fileSize;
  } else {
    readSize = bufSize;
  }

  freadSize = fread(buf, sizeof(buf[0]), readSize, fp);
  if ((freadSize != readSize) && !(ftell(fp) == fileSize)) {
    printf("failed to fread %ld bytes\n", readSize);
    goto done;
  }

  retval = 0;

done:
  return retval;
}

int32_t IsDolbyVisionOBU(uint8_t *buf, const int64_t bufSize,
                         const OBPOBUType obu_type, OBPError *err,
                         bool *isDolbyVision) {
  *isDolbyVision = false;
  int32_t retval = 0;

  if (OBP_OBU_METADATA != obu_type) {
    goto done;
  }

  OBPMetadata metadata = {0};
  retval = obp_parse_metadata(buf, bufSize, &metadata, err);
  if (0 != retval) {
    printf("error obp_parse_metadata\n");
    printf("%s\n", err->error);
    goto done;
  }

  static const uint8_t itu_t_t35_country_code_usa = 0xB5;
  static const uint8_t itu_t_t35_payload_bytes_dv[] = {0x00, 0x3B};
  bool is_usa_cc = itu_t_t35_country_code_usa ==
                   metadata.metadata_itut_t35.itu_t_t35_country_code;
  bool is_dv_payload = false;
  if (metadata.metadata_itut_t35.itu_t_t35_payload_bytes_size >= 2) {
    is_dv_payload = (itu_t_t35_payload_bytes_dv[0] ==
                     metadata.metadata_itut_t35.itu_t_t35_payload_bytes[0]) &&
                    (itu_t_t35_payload_bytes_dv[1] ==
                     metadata.metadata_itut_t35.itu_t_t35_payload_bytes[1]);
  }
  if (is_usa_cc && is_dv_payload) {
    *isDolbyVision = true;
  }

done:
  return retval;
}

void PrintProgress(const size_t current, const size_t total,
                                 const uint32_t numObuDV,
                                 const time_t startTime) {
  time_t timeNow = time(NULL);
  double timeElapsed = difftime(timeNow, startTime);
  double progress = (double)current / (double)total;
  double estimatedTotalTime = timeElapsed / progress;
  double ETA = estimatedTotalTime - timeElapsed;

  printf("\r                                             ");
  printf("\r%.2f%%, ETA: %.0f, %d DV OBU", (progress * 100), ETA, numObuDV);
  fflush(stdout);
  return;
}

int32_t main(int32_t argc, char **argv) {
  int32_t retval = 0;
  static FILE *inFP = NULL;
  static FILE *outFP = NULL;
  static uint8_t *buf = NULL;

  // min args:
  // -i input
  if (3 > argc) {
    usage(argv[0]);
    retval = 1;
    goto done;
  }

  static char *inPath = NULL;
  static char *outPath = NULL;

  for (int32_t i = 0; i < argc; i++) {
    if (0 == strcmp(argv[i], "-i")) {
      if ((argc - 1) == i) {
        printf("no argument for input file\n");
        usage(argv[0]);
        retval = 1;
        goto done;
      }
      inPath = argv[i + 1];
    } else if (0 == strcmp(argv[i], "-c")) {
      if ((argc - 1) == i) {
        printf("no argument for chunk size\n");
        usage(argv[0]);
        retval = 1;
        goto done;
      }
      CHUNK_SIZE = atoi(argv[i + 1]);
    } else if (0 == strcmp(argv[i], "-o")) {
      if ((argc - 1) == i) {
        printf("no argument for output file\n");
        usage(argv[0]);
        retval = 1;
        goto done;
      }
      outPath = argv[i + 1];
    }
  }

  // open input
  inFP = fopen(inPath, "r");
  if (NULL == inFP) {
    perror("could not open input file");
    retval = 1;
    goto done;
  }

  // get size of input
  fseek(inFP, 0, SEEK_END);
  const size_t inputSize = ftell(inFP);
  rewind(inFP);

  printf("input:%s\n", inPath);
  printf("input size:%ld\n", inputSize);
  printf("chunk size:%ld\n", CHUNK_SIZE);
  printf("output:%s\n", outPath);

  // create buffer
  buf = (uint8_t *)calloc(sizeof(uint8_t), CHUNK_SIZE);
  if (NULL == buf) {
    perror("could not calloc for buf");
    retval = 1;
    goto done;
  }

  retval = ReadNextChunk(&buf[0], CHUNK_SIZE, CHUNK_SIZE, inFP, inputSize);
  if (0 != retval) {
    printf("error ReadNextChunk\n");
    retval = 1;
    goto done;
  }

  // open optional output
  if (NULL != outPath) {
    outFP = fopen(outPath, "w+");
    if (NULL == outFP) {
      perror("could not open output file");
      retval = 1;
      goto done;
    }
  }

  static OBPOBUType obu_type = {0};
  static ptrdiff_t offset = {0};
  static size_t obu_size = 0;
  static int temporal_id = 0;
  static int spatial_id = 0;
  // setup error message buffer
  static char errBuf[ERROR_BUFFER_SIZE] = {0};
  static OBPError err = {.error = &errBuf[0], .size = sizeof(errBuf)};

  const time_t startTime = time(NULL);
  static uint64_t numObu = 0;
  static uint64_t numObuDV = 0;
  static size_t bufInd = 0;
  static size_t numBytesRead = 0;
  size_t remainingBufSize = (CHUNK_SIZE - bufInd);

  while (numBytesRead < inputSize) {
    PrintProgress(numBytesRead, inputSize, numObuDV, startTime);

    // pre-emptive reading of next chunk
    if ((bufInd * 2) > CHUNK_SIZE) {
      memmove(&buf[0], &buf[bufInd], remainingBufSize);
      bufInd = 0;
      size_t freeBufSpace = CHUNK_SIZE - remainingBufSize;
      retval = ReadNextChunk(&buf[remainingBufSize], freeBufSpace, CHUNK_SIZE,
                             inFP, inputSize);
      if (0 != retval) {
        printf("error ReadNextChunk\n");
        printf("bufInd:%ld\n", bufInd);
        printf("remainingBufSize:%ld\n", remainingBufSize);
        retval = 1;
        goto done;
      }
      remainingBufSize = CHUNK_SIZE - bufInd;
    }

    // find next obu header
    retval =
        obp_get_next_obu(&buf[bufInd], remainingBufSize, &obu_type, &offset,
                         &obu_size, &temporal_id, &spatial_id, &err);
    if (0 != retval) {
      // read next chunk if next OBU is possibly in next chunk
      if ((obu_size < CHUNK_SIZE) && (obu_size > remainingBufSize)) {
        memmove(&buf[0], &buf[bufInd], remainingBufSize);
        bufInd = 0;
        size_t freeBufSpace = CHUNK_SIZE - remainingBufSize;
        retval = ReadNextChunk(&buf[remainingBufSize], freeBufSpace, CHUNK_SIZE,
                               inFP, inputSize);
        if (0 != retval) {
          printf("error ReadNextChunk\n");
          printf("bufInd:%ld\n", bufInd);
          printf("remainingBufSize:%ld\n", remainingBufSize);
          retval = 1;
          goto done;
        }
        remainingBufSize = CHUNK_SIZE - bufInd;
        // reset loop to retry
        continue;
      }
      printf("\n\nerror obp_get_next_obu\n");
      printf("%s\n", errBuf);
      retval = 1;
      printf("remaining buffer size:%ld\n", remainingBufSize);
      printf("OBU size:%ld\n", obu_size);
      printf("try increasing chunk size\n");
      break;
    }

    const size_t totalObuSize = offset + obu_size;

    // check if DV
    bool isDolbyVision = false;
    size_t obuDataInd = bufInd + offset;
    retval = IsDolbyVisionOBU(&buf[obuDataInd], (remainingBufSize - offset),
                              obu_type, &err, &isDolbyVision);
    if (0 != retval) {
      printf("error IsDolbyVisionOBU\n");
      retval = 1;
      break;
    }
    if (isDolbyVision) {
      numObuDV++;
    } else if (NULL != outFP) {
      size_t fwriteSize =
          fwrite(&buf[bufInd], sizeof(buf[0]), totalObuSize, outFP);
      if (totalObuSize != fwriteSize) {
        printf("error fwrite wrote %ld/%ld bytes\n", fwriteSize, totalObuSize);
        retval = 1;
        break;
      }
    }

    // jump to next OBU
    bufInd += totalObuSize;
    numBytesRead += totalObuSize;
    remainingBufSize = CHUNK_SIZE - bufInd;

    numObu++;
    if (numBytesRead == inputSize) {
      PrintProgress(numBytesRead, inputSize, numObuDV, startTime);
    }
  }

  printf("\r                                             \r");
  printf("total OBU read:%ld\n", numObu);
  printf("total DV OBU read:%ld\n", numObuDV);
  time_t timeNow = time(NULL);
  double timeElapsed = difftime(timeNow, startTime);
  printf("total processing time:%.0f seconds\n", timeElapsed);

done:
  if (NULL != buf) {
    free(buf);
  }
  if (NULL != inFP) {
    fclose(inFP);
    inFP = NULL;
  }
  if (NULL != outFP) {
    fclose(outFP);
    outFP = NULL;
  }

  return retval;
}