
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

int TreBuffer_ReadCharAtCursor(TreBuffer* buf) {
  return TRE_Buf_read_char_at_cursor((TRE_Buf*)buf);
}

/*
TODO: Change basic buffer methods (move charwise/linewise) so that they return
new positions as return values instead of mutating the buffer. Then add
additional methods that get the new position AND mutate the buffer. Then the
basic movement functions can be more easily used for other editing purposes.

Compare to Scintilla:
http://docs.wxwidgets.org/trunk/classwx_styled_text_ctrl.html

More buffer methods to consider:

Get buffer length
Get/set selection, jump to opposite end of selection, select all, clear select
Marks?
Read-only status
Get line length
Set/clear dirty
Undo/redo, set save point, checkpointing (undo history as series of diffs vs
series of specific events?)
Revert?
Convert between absolute position and row/col position
Go to line/col
Search/replace (in range)
Get text in range
Delete text in range
Replace text range with string (maybe do as atom operation for undo purposes)
Get/insert/delete line
Get/set EOL mode
Expand tab, tab width
Markers: like N++ highlights. Scintilla lets you search for them using a mask.
Could be useful for driving various UI functionality?
Count chars in range
Home, end, doc home, doc end

Possible UI functionality:
Styling? Set style in range, get style at point, set default style
Drag/drop
Scrolling: scroll n lines/chars, scroll to line/col
View/hide whitespace
Translate x,y positions to/from text positions (close only vs. allow all)
Event hooks, key bindings
Scroll to center range in view
Scroll to cursor position
Cut/paste

Not sure what level to implement:
Syntax highlighting (the highlighting itself, syntax analysis is a separate
component)
Move in content-aware ways
Indent/unindent
Search
Line split, join, wrap
Move by word

Other functionality:
Compiler integration
Language-aware movement
Word characters (for word selection)
Auto-complete
Autoformat/autoindent
*/

