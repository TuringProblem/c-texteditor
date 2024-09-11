#include "editor.h"
#include <asm-generic/errno-base.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct editorConfig E;

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
  perror(s);
  exit(1);
}

void disableRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.origTermios) == -1)
    die("tcsetattr");
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.origTermios) == -1)
    die("tcgetattr");
  atexit(disableRawMode);
  struct termios raw = E.origTermios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

char editorReadKey() {
  int nRead;
  char c;
  while ((nRead == read(STDIN_FILENO, &c, 1)) != 1) {
    if (nRead == -1 && errno != EAGAIN)
      die("read");
  }
  return c;
}

void editorInsertChar(int c) {
  if (E.cy == E.numRows) {
    E.row = realloc(E.row, sizeof(erow) * (E.numRows + 1));
    E.row[E.numRows].chars = malloc(1);
    E.row[E.numRows].length = 0;
    E.numRows++;
  }
  erow *row = &E.row[E.cy];
  row->chars = realloc(row->chars, row->length + 2);
  memmove(&row->chars[E.cx + 1], &row->chars[E.cx], row->length - E.cx + 1);
  row->length++;
  row->chars[E.cx] = c;
  E.cx++;
}

void editorMoveCursor(char key) {
  erow *row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];
  switch (key) {
  case 'h':
    if (E.cx != 0) {
      E.cx--;
      E.cx = E.row[E.cy].length;
    }
    break;
  case 'j':
    if (E.cy < E.numRows) {
      E.cy++;
    }
    break;
  case 'k':
    if (E.cy != 0) {
      E.cy--;
    }
    break;
  case 'l':
    if (row && E.cx < row->length) {
      E.cx++;
    } else if (row && E.cx == row->length && E.cy < E.numRows - 1) {
      E.cy++;
      E.cx = 0;
    }
    break;
  }
  row = (E.cy >= E.numRows) ? NULL : &E.row[E.cy];
  int rowlen = row ? row->length : 0;
  if (E.cx > rowlen)
    E.cx = rowlen;
}

void editorDrawRows() {
  int i;
  for (i = 0; i < E.numRows; i++) {
    write(STDOUT_FILENO, E.row[i].chars, E.row[i].length);
    write(STDOUT_FILENO, "\r\n", 2);
  }
}

int editorProcessNormalMode() {
  char c = editorReadKey();
  switch (c) {
  case ':':
    strcpy(E.statusmsg, ":");
    return 1;
  case 'i':
    E.mode = MODE_INSERT;
    strcpy(E.statusmsg, "-- INSERT --");
    return 1;
  case 'h':
  case 'j':
  case 'k':
  case 'l':
    editorMoveCursor(c);
    return 1;
  default:
    return 1;
  }
}

int editorProcessKeypress() {
  char c = editorReadKey();
  switch (c) {
  case ':':
    strcpy(E.statusmsg, ":");
    return 1;
  case 'i':
    E.mode = MODE_INSERT;
    strcpy(E.statusmsg, "-- INSERT --");
    return 1;
  case 'h':
  case 'j':
  case 'k':
  case 'l':
    editorMoveCursor(c);
    return 1;
  default:
    return 1;
  }
}

void editorDrawStatusBar() {
  char status[80];
  int len = snprintf(status, sizeof(status), "%.20s - %s mode", E.statusmsg,
                     E.mode == MODE_NORMAL ? "NORMAL" : "INSERT");
  write(STDOUT_FILENO, "\x1b[7m", 4);
  write(STDOUT_FILENO, status, len);
  write(STDOUT_FILENO, "\x1b[m", 3);
}

void editorRefreshScreen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  editorDrawStatusBar();

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  write(STDOUT_FILENO, buf, strlen(buf));
}

int editorProcessCommand() {
  static char command[80] = {0};
  static int commandPos = 0;
  char c = editorReadKey();
  if (c == '\r') {
    command[commandPos] = '\0';
    if (strcpy(E.statusmsg, ":q") == 0) {
      return 0;
    }

    // resetting the command buffer
    commandPos = 0;
    strcpy(E.statusmsg, "");
  } else if (isprint(c)) {
    command[commandPos++] = c;
    command[commandPos] = '\0';
    strcpy(E.statusmsg, command);
  }
  return 1;
}

void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.mode = MODE_NORMAL;
  strcpy(E.statusmsg, "");
}

int main() {
  enableRawMode();
  initEditor();
  while (1) {
    editorRefreshScreen();
    if (!editorProcessKeypress())
      break;
  }
  return 0;
}
