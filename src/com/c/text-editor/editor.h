#ifndef EDITOR_H
#define EDITOR_H

#include <termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)

enum editorMode { MODE_NORMAL, MODE_INSERT };

#define EDITOR_MAX_LINES 1000
#define EDITOR_MAX_LINE_LENGTH 1000

typedef struct {
  char *chars;
  int length;
} erow;

struct editorConfig {
  int cx, cy;
  int numRows;
  erow *row;
  enum editorMode mode;
  struct termios origTermios;
  char statusmsg[80];
};

extern struct editorConfig E;

void die(const char *s);
void enableRawMode();
void editorMoveCursor(char key);
void editorRefreshScreen();
int editorProcessKeypress();
void initEditor();

#endif
