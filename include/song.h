#ifndef _song
#define _song

// Data pair struct
typedef struct DataPair {
  uint32_t delta_time;
  uint8_t key;
} DataPair;

// Track chunk struct
typedef struct TrackChunk {
  uint32_t length;
  uint32_t tempo;
  DataPair **pairs;
} TrackChunk;

// Header chunk struct
typedef struct HeaderChunk {
  uint32_t length;
  uint16_t format;
  uint16_t tracks;
  uint16_t ppq;
} HeaderChunk;

// Song struct
typedef struct Song {
  HeaderChunk *header;
  TrackChunk *tempo;
  TrackChunk *data;
} Song;

#endif
