/*
** data.h — Position Data Model
**
** Pure data types and initialization. No UI dependency.
** In production, these structs would be populated from a socket/shm feed.
*/

#ifndef DATA_H
#define DATA_H

#define MAX_POSITIONS 128

/* ---- Asset Classification ---- */
typedef enum {
  ASSET_GOVT_BOND,
  ASSET_IRS,
  ASSET_FRA,
  ASSET_FUTURES,
  ASSET_SWAPTION,
  ASSET_CLASS_COUNT
} AssetClass;

static const char *ASSET_CLASS_NAMES[] = {
  "Bond", "IRS", "FRA", "Futures", "Swaption"
};

/* ---- Single Position ---- */
typedef struct {
  const char *instrument;
  const char *cusip;
  AssetClass  asset_class;
  const char *book;
  const char *desk;
  double      notional;        /* millions                */
  double      avg_price;
  double      mkt_price;
  double      pnl_total;       /* total P&L in thousands  */
  double      pnl_day;         /* day P&L in thousands    */
  double      dv01;
  double      cs01;
  double      delta;
  double      gamma;
  double      vega;
  double      theta;
  int         stale;           /* stale price flag        */
} Position;

/* ---- Global Position Book ---- */
typedef struct {
  Position items[MAX_POSITIONS];
  int      count;
} PositionBook;

/* ---- Initialize with realistic rates desk data ---- */
void data_init(PositionBook *book);

/* ---- Simulate market data tick (random walk + theta bleed) ---- */
void data_simulate_tick(PositionBook *book, int tick);

#endif
