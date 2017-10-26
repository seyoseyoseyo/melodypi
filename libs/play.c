// Libraries
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

// Dependencies
#include "play.h"
#include "song.h"
#include "thpool.h"
#include "wiringPi.h"

// Auxiliary function declarations
static void setup(void);
static void playAll(void *key_pointer);
static void playNote(int key);

// External variables
extern uint32_t event_counter;
extern Song *song;
extern threadpool thpool;

// Plays a song
void play(void *pointer) {
  // Set up GPIO pins
  setup();
  int counter = 0;
  // Time division in microseconds
  uint32_t time_div = 60000000 / (song->header->ppq * song->tempo->tempo);
  // Song playback begins
  printf("Playback begins\n");
  while (counter < event_counter) {
    // Time in microseconds
    double time = song->data->pairs[counter]->delta_time * time_div;
    // Rest for time in seconds
    printf("Rest for %f sec\n", time / 1000000);
    usleep(time);
    if (song->data->pairs[counter]->delta_time != 0) {
      printf("\n");
    }
    // Add thread to threadpool
    thpool_add_work(thpool, &playAll, &(song->data->pairs[counter]->key));
    counter++;
  }
  // Safety wait, then destroy threadpool
  thpool_wait(thpool);
  thpool_destroy(thpool);
  // Song playback finished
  printf("Playback finished\n");
}

// Setups GPIO pins and sets pin modes
static void setup(void) {
  // Set up wiring pi pins
  wiringPiSetup();
  // Set pin modes to output
  for (int i = 0; i < 8; i++) {
    pinMode(i, OUTPUT);
  }
  for (int i = 21; i < 28; i++) {
    pinMode(i, OUTPUT);
  }
}

// Plays all notes through pins
static void playAll(void *key_pointer) {
  uint8_t key = *((int *) key_pointer);
  if (key < 43 || key > 67) {
    printf("Note: %d outside of range, ignored\n", key);
    return;
  }
  int octave = key / 12;
  switch (key % 12) {
    case 0: // C
      printf("Playing C%i now\n", octave);
      playNote(octave == 4 ? 3 : 23);
      break;
    case 1: // C#
      printf("Note C# unimplemented\n");
      return;
    case 2: // D
      printf("Playing D%i now\n", octave);
      playNote(octave == 4 ? 4 : 24);
      break;
    case 3: // D#
      printf("Note D# unimplemented\n");
      break;
    case 4: // E
      printf("Playing E%i now\n", octave);
      playNote(octave == 4 ? 5 : 25);
      break;
    case 5: // F
      printf("Playing F%i now\n", octave);
      playNote(octave == 4 ? 6 : 26);
      break;
    case 6: // F#
      printf("Note F# unimplemented\n");
      break;
    case 7: // G
      printf("Playing G%i now\n", octave);
      playNote(octave == 4 ? 7 : octave == 5 ? 27 : 0);
      break;
    case 8: // G#
      printf("Note G# unimplemented\n");
      break;
    case 9: // A
      printf("Playing A%i now\n", octave);
      playNote(octave == 3 ? 1 : 21);
      break;
    case 10: // A#
      printf("Note A# unimplemented\n");
      break;
    case 11: // B
      printf("Playing B%i now\n", octave);
      playNote(octave == 3 ? 2 : 22);
      break;
    default:
      printf("Unknown note\n");
      break;
  }
}

// Plays a note through a pin
static void playNote(int pin) {
  digitalWrite(pin, HIGH);
  delay(80);
  digitalWrite(pin, LOW);
}
