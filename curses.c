
#include "hdrs.c"

/*
#include "libtermkey/termkey.h"
#include "mh_curses.h"

TermKey* termkey;

TRE_OpResult init_terminal(void)
{
  // Init termkey.
  TERMKEY_CHECK_VERSION;
  if (NULL == (termkey = termkey_new(0, 0))) {
    return TRE_FAIL;
  }
  // Init curses.
  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();
  // TODO: Set up window(s)
  atexit(cleanup);
  return TRE_SUCC;
}

LOCAL void cleanup(void)
{
  endwin();
  termkey_destroy(termkey);
}

int read_char(void) {
  return getch();
}

#if 0
LOCAL char* modifiers(int mods) {
  static char buf[64];
  char* p = buf;
  *p = 0;
  if (mods & TERMKEY_KEYMOD_CTRL) {
    p += sprintf(buf, "Ctrl-");
  }
  if (mods & TERMKEY_KEYMOD_ALT) {
    p += sprintf(buf, "Alt-");
  }
  if (mods & TERMKEY_KEYMOD_SHIFT) {
    p += sprintf(buf, "Shift-");
  }
  return buf;
}

int read_char(void)
{
  TermKeyKey key;
  TermKeyResult r = termkey_waitkey(termkey, &key);
  if (TERMKEY_RES_KEY == r) {
    switch (key.type) {
      case TERMKEY_TYPE_UNICODE:
        if (isprint(key.code.codepoint)) {
          logt("Received codepoint: %d (%s%c)", key.code.codepoint,
              modifiers(key.modifiers), key.code.codepoint);
        } else {
          logt("Received codepoint: %d", key.code.codepoint);
        }
        return key.code.codepoint;
      case TERMKEY_TYPE_KEYSYM:
        logt("Received sym: %d (%s%s)", key.code.sym, modifiers(key.modifiers),
            termkey_get_keyname(termkey, key.code.sym));
        return key.code.sym;
      case TERMKEY_TYPE_FUNCTION:
        logt("Received F key: F%d", key.code.number);
        return KEY_F(key.code.number);
      case TERMKEY_TYPE_MOUSE:
      default:
        logt("Received another key event.");
        return KEY_F(12);
    }
  } else if (TERMKEY_RES_ERROR == r) {
    log_err("Termkey reported an error.");
    return -1;
  } else if (TERMKEY_RES_EOF == r) {
    log_err("Termkey reported EOF.");
    return -1;
  } else {
    log_err("Unexpected return value from termkey.");
    return -1;
  }
}
#endif
*/


