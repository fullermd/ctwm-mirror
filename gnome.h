
typedef struct {
  Window *ws;
  int wsSize;
  int numWins;
} GnomeData;

void InitGnome ();
void GnomeAddClientWindow ();
void GnomeDeleteClientWindow ();

