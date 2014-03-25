
#include "hdrs.c"
#include "mh_api.h"

#if INTERFACE
typedef struct TreBuffer {
  char dummy[1];
} TreBuffer;

typedef struct TreCursorPosition {
  int cursor_line;
  int cursor_column;
  int cursor_offset;
} TreCursorPosition;
#endif

//--------------------------------------------------------------------

void TreBuffer_MoveCharwise(TreBuffer* buf, int distanceChars) {
  TRE_Buf_move_charwise((TRE_Buf*)buf, distanceChars);
}

void TreBuffer_MoveLinewise(TreBuffer* buf, int distanceLines) {
  TRE_Buf_move_linewise((TRE_Buf*)buf, distanceLines);
}

TRE_OpResult TreBuffer_SetCursorPosition(TreBuffer* buf, int absolutePosition) {
  return TRE_Buf_move_gap((TRE_Buf*)buf, absolutePosition);
}

TreCursorPosition TreBuffer_GetCursorPosition(TreBuffer* buf) {
  TRE_Buf* bufptr = (TRE_Buf*)buf;
  TreCursorPosition pos;
  pos.cursor_line = bufptr->cursor_line.num;
  pos.cursor_column = bufptr->cursor_col;
  pos.cursor_offset = bufptr->gap_start;
  return pos;
}

void TreBuffer_InsertChar(TreBuffer* buf, int c) {
  assert(c >= 0);
  assert(c < 256);
  TRE_Buf_insert_char((TRE_Buf*)buf, (char)c);
}

void TreBuffer_InsertString(TreBuffer* buf, const char* str) {
  TRE_Buf_insert_string((TRE_Buf*)buf, str);
}

void TreBuffer_Delete(TreBuffer* buf) {
  TRE_Buf_delete((TRE_Buf*)buf);
}

void TreBuffer_Backspace(TreBuffer* buf) {
  TRE_Buf_backspace((TRE_Buf*)buf);
}

void TreBuffer_ReadCharAtCursor(TreBuffer* buf) {
  return TRE_Buf_read_char_at_cursor((TRE_Buf*)buf);
}

