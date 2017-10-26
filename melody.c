// Libraries
#include <assert.h>
#include <stdlib.h>

// Dependencies
#include "bit_utils.h"
#include "play.h"
#include "song.h"
#include "thpool.h"

// Input file
FILE *input;

// Song struct
Song *song;

// Byte counter
int byte_counter;

// Delta time accumulator
uint32_t delta_time;

// Event counter
uint32_t event_counter;

// Threadpools
threadpool thpool;

// Function declarations
int main(int argc, char **argv);

// Auxiliary function declarations
static uint8_t nextByte(void);
static void allocSong(void);
static void readHeader(void);
static void readTempo(void);
static void readData(void);
static void parseData(uint8_t command);
static void parseMetaEvent(uint8_t command);
static void transpose(int semitone);
static void addDeltaTime(void);
static void freeSong(void);

// Main function
int main(int argc, char **argv) {
  // If argument count is not 1, do not run
  if (argc <= 1) {
    printf("Please have at least one .MID file as argument\n");
    exit(EXIT_FAILURE);
  }
  // Load file
  input = fopen(argv[1], "rb");
  if (input == NULL) {
    perror("File not found");
    exit(EXIT_FAILURE);
  }
  // Initialisations
  allocSong();
  thpool = thpool_init(6);
  byte_counter = 0;
  delta_time = 0;
  event_counter = 0;
  // Read all chunks
  readHeader();
  readTempo();
  readData();
  // If argument count is 2, transpose by semitone
  if (argc == 3) {
    transpose(atoi(argv[2]));
  }
  // Playback song by adding play thread to threadpool
  thpool_add_work(thpool, &play, NULL);
  // Check for interrupt/pause signals
  while (thpool_num_threads_working(thpool) != 0) {
  }
  // Safety wait, then destroy threadpool
  thpool_wait(thpool); 
  thpool_destroy(thpool);
  // Free song memory allocation
  freeSong();
  // Unload file
  fclose(input);
  return EXIT_SUCCESS;
}

// Reads the next line from input file
static uint8_t nextByte(void) {
  uint8_t message = 0;
  fread(&message, 1, 1, input);
  byte_counter++;
  return message;
}

// Allocates memory for a song
static void allocSong(void) {
  song = calloc(1, sizeof(Song));
  song->header = calloc(1, sizeof(HeaderChunk));
  song->tempo = calloc(1, sizeof(TrackChunk));
  song->data = calloc(1, sizeof(TrackChunk));
}

// Reads the header chunk
static void readHeader(void) {
  // Assert marker
  assert(nextByte() == 0x4d && nextByte() == 0x54
      && nextByte() == 0x68 && nextByte() == 0x64 && "Header chunk not found");
  // Read header chunk binary format
  song->header->length = 
      nextByte() << 24 | nextByte() << 16 | nextByte() << 8 | nextByte();
  song->header->format = nextByte() << 8 | nextByte();
  song->header->tracks = nextByte() << 8 | nextByte();
  song->header->ppq = nextByte() << 8 | nextByte();
}

// Reads the tempo track
static void readTempo(void) {
  // Assert marker
  assert(nextByte() == 0x4d && nextByte() == 0x54
      && nextByte() == 0x72 && nextByte() == 0x6b && "Tempo track not found");
  // Read tempo track binary format
  song->tempo->length = 
      nextByte() << 24 | nextByte() << 16 | nextByte() << 8 | nextByte();
  // Reset byte counter
  byte_counter = 0;
  // Parse remainder binary commands
  addDeltaTime();
  while (byte_counter < song->tempo->length) {
    parseMetaEvent(nextByte());
  }
}

// Reads the data track
static void readData() {
  // Assert marker
  assert(nextByte() == 0x4d && nextByte() == 0x54
      && nextByte() == 0x72 && nextByte() == 0x6b && "Data track not found");
  // Read data track binary format
  song->data->length = 
      nextByte() << 24 | nextByte() << 16 | nextByte() << 8 | nextByte();
  // Initialise data pairs
  song->data->pairs = calloc(song->data->length, sizeof(DataPair));
  // Reset byte counter
  byte_counter = 0;
  // Parse remainder binary commands
  addDeltaTime();
  while (byte_counter < song->data->length) {
    parseData(nextByte());
  }
}

// Parses a data and its parameters
static void parseData(uint8_t command) {
  switch (command >> (BITS_PER_BYTE >> 1)) {
    case 0x9:;
      // Get next two bytes for two parameters
      uint8_t key = nextByte();
      nextByte();
      // Store in data pair struct
      DataPair *pair = calloc(1, sizeof(DataPair));
      pair->delta_time = delta_time;
      pair->key = key;
      song->data->pairs[event_counter] = pair;
      event_counter++;
      // Reset delta time accumulator
      delta_time = 0;
      break;
    case 0x8:
    case 0xA:
    case 0xB:
    case 0xE:
      nextByte();
    case 0xC:
    case 0xD:
      nextByte();
      break;
    case 0xF:
      parseMetaEvent(command);
      break;
    default:
      break;
  }
  addDeltaTime();
}

// Parses a meta event and its parameters
static void parseMetaEvent(uint8_t command) {
  if (command == 0xFF) {
    switch (nextByte()) {
      case 0x51:
        if (nextByte() == 0x03) {
          song->tempo->tempo = 60000000 /
              (nextByte() << 16 | nextByte() << 8 | nextByte());
        }
        break;
      case 0x54:
        nextByte();
      case 0x58:
        nextByte();
        nextByte();
      case 0x00:
      case 0x59:
        nextByte();
      case 0x20:
        nextByte();
      case 0x2F:
        nextByte();
        break;
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x06:
      case 0x07:
      case 0x7F:;
        int length = nextByte();
        for (int counter = 0; counter < length; counter++) {
          nextByte();
        }
        break;
      default:
        break;
    }
  }
}

// Transposes by semitone
static void transpose(int semitone) {
  for (int i = 0; i < event_counter; i++) {
    song->data->pairs[i]->key += semitone;
  }
}

// Adds delta time to accumulator
static void addDeltaTime(void) {
  uint8_t byte = nextByte();
  uint32_t time = byte;
  while (getBit(byte, 7) == 1) {
    byte = nextByte();
    time = getBits(byte, 0, 6) | getBits(time, 0, 6) << (BITS_PER_BYTE - 1);
  }
  delta_time += time;
}

// Frees memory allocation of song
static void freeSong(void) {
  for (int i = 0; i < event_counter; i++) {
    free(song->data->pairs[i]);
  }
  free(song->data->pairs);
  free(song->data);
  free(song->tempo);
  free(song->header);
  free(song);
}
