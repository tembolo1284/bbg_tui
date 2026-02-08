/*
** data.c — Position data and market simulation
*/

#include <stdlib.h>
#include <string.h>
#include "data.h"

void data_init(PositionBook *book) {
  memset(book, 0, sizeof(*book));

  struct {
    const char *inst; const char *cusip; AssetClass ac;
    const char *book; const char *desk;
    double notl, avg, mkt, pnl, pnl_d, dv01, cs01;
    double delta, gamma, vega, theta;
    int stale;
  } seed[] = {
    /* ---- Rates Flow / US Rates ---- */
    { "UST 2Y 4.25 03/27",   "91282CKL8",    ASSET_GOVT_BOND, "Rates Flow", "US Rates",
       150, 99.875, 99.920,  67.5,  12.3, 2850, 0,  0.98, 0.001, 0, -0.45, 0 },
    { "UST 5Y 4.00 02/30",   "91282CKM6",    ASSET_GOVT_BOND, "Rates Flow", "US Rates",
      -75, 98.500, 98.125, -28.1, -15.7, 3520, 0, -0.95, 0.003, 0, -1.12, 0 },
    { "UST 10Y 3.875 11/34",  "91282CKN4",   ASSET_GOVT_BOND, "Rates Flow", "US Rates",
       200, 97.250, 97.750, 100.0,  35.2, 8750, 0,  0.92, 0.008, 0, -2.80, 0 },
    { "UST 30Y 4.25 05/54",   "91282CKP9",   ASSET_GOVT_BOND, "Rates Flow", "US Rates",
        50, 96.125, 95.875, -12.5,  -8.1, 15200, 0, 0.88, 0.022, 0, -5.10, 1 },
    /* ---- Rates Flow / EUR Rates ---- */
    { "DBR 2Y 2.80 12/26",   "DE000BU2Z023", ASSET_GOVT_BOND, "Rates Flow", "EUR Rates",
       100, 100.125, 100.250, 12.5,   3.2, 1920, 0,  0.99, 0.001, 0, -0.22, 0 },
    { "DBR 10Y 2.50 08/33",  "DE000BU2Z031", ASSET_GOVT_BOND, "Rates Flow", "EUR Rates",
      -120, 98.750, 99.125, -45.0,  -9.4, 7800, 0, -0.93, 0.007, 0, -2.40, 0 },
    { "OAT 10Y 3.00 05/34",  "FR0014007L89", ASSET_GOVT_BOND, "Rates Flow", "EUR Rates",
        80, 99.000, 98.500, -40.0,  -5.6, 7200, 120, 0.91, 0.006, 0, -2.10, 0 },
    { "GILT 10Y 4.50 09/34",  "GB00BMBL1F74", ASSET_GOVT_BOND, "Rates Flow", "GBP Rates",
        60, 101.250, 101.500, 15.0,   4.8, 7100, 0,  0.93, 0.006, 0, -2.30, 0 },
    /* ---- Swaps ---- */
    { "IRS USD 2Y vs 3M",    "N/A",          ASSET_IRS,       "Swaps",      "USD Swaps",
       300, 4.450, 4.420, 90.0,   7.5, 5800, 0, 0, 0, 0, -0.80, 0 },
    { "IRS USD 5Y vs 3M",    "N/A",          ASSET_IRS,       "Swaps",      "USD Swaps",
       500, 4.250, 4.180, 350.0,  22.1, 22100, 0, 0, 0, 0, -3.20, 0 },
    { "IRS USD 10Y vs 3M",   "N/A",          ASSET_IRS,       "Swaps",      "USD Swaps",
      -250, 4.050, 4.120, -175.0, -18.5, 23500, 0, 0, 0, 0, -6.80, 0 },
    { "IRS USD 30Y vs 3M",   "N/A",          ASSET_IRS,       "Swaps",      "USD Swaps",
       100, 3.950, 3.980, -30.0,  -2.1, 28000, 0, 0, 0, 0, -9.50, 0 },
    { "IRS EUR 5Y vs 6M",    "N/A",          ASSET_IRS,       "Swaps",      "EUR Swaps",
       300, 2.850, 2.780, 210.0,  15.4, 14200, 0, 0, 0, 0, -2.10, 0 },
    { "IRS EUR 10Y vs 6M",   "N/A",          ASSET_IRS,       "Swaps",      "EUR Swaps",
       150, 2.750, 2.810, -90.0,  -7.2, 14800, 0, 0, 0, 0, -4.50, 1 },
    { "IRS GBP 5Y vs SONIA", "N/A",          ASSET_IRS,       "Swaps",      "GBP Swaps",
       200, 4.100, 4.050, 100.0,   8.9, 10500, 0, 0, 0, 0, -1.80, 0 },
    /* ---- Futures ---- */
    { "TU  H6 (2Y Fut)",     "N/A",          ASSET_FUTURES,   "Futures",    "US Rates",
       400, 103.140, 103.200, 24.0,   8.5, 1650, 0,  0.99, 0, 0, -0.10, 0 },
    { "FV  H6 (5Y Fut)",     "N/A",          ASSET_FUTURES,   "Futures",    "US Rates",
      -300, 108.250, 108.125, -37.5, -6.2, 4200, 0, -0.97, 0, 0, -0.35, 0 },
    { "TY  H6 (10Y Fut)",    "N/A",          ASSET_FUTURES,   "Futures",    "US Rates",
       200, 111.500, 112.000, 100.0, 28.3, 7800, 0,  0.94, 0, 0, -0.90, 0 },
    { "US  H6 (Bond Fut)",   "N/A",          ASSET_FUTURES,   "Futures",    "US Rates",
       -50, 118.750, 118.500, 12.5,   4.1, 13200, 0, -0.90, 0, 0, -1.50, 0 },
    { "RX  H6 (Bund Fut)",   "N/A",          ASSET_FUTURES,   "Futures",    "EUR Rates",
       150, 131.250, 131.500, 37.5,  11.2, 7500, 0,  0.93, 0, 0, -0.70, 0 },
    { "OAT H6 (OAT Fut)",   "N/A",          ASSET_FUTURES,   "Futures",    "EUR Rates",
        80, 126.500, 126.750, 20.0,   6.4, 6800, 0,  0.92, 0, 0, -0.60, 0 },
    /* ---- Vol Desk / Swaptions ---- */
    { "USD 1Yx5Y Payer",     "N/A",          ASSET_SWAPTION,  "Vol Desk",   "USD Vol",
       100, 0, 0,  45.0,  5.8, 4200, 0,  0.52, 0.08, 285.0, -12.5, 0 },
    { "USD 5Yx10Y Recv",     "N/A",          ASSET_SWAPTION,  "Vol Desk",   "USD Vol",
       -80, 0, 0, -22.0, -3.1, 8900, 0, -0.48, 0.05, 420.0, -18.2, 0 },
    { "EUR 1Yx10Y Payer",    "N/A",          ASSET_SWAPTION,  "Vol Desk",   "EUR Vol",
        60, 0, 0,  18.5,  2.4, 5600, 0,  0.55, 0.06, 310.0, -14.8, 1 },
    { "USD 3Mx10Y Straddle", "N/A",          ASSET_SWAPTION,  "Vol Desk",   "USD Vol",
       120, 0, 0,  32.0,  4.2, 6200, 0,  0.02, 0.12, 550.0, -22.0, 0 },
  };

  int n = sizeof(seed) / sizeof(seed[0]);
  book->count = n;

  for (int i = 0; i < n; i++) {
    book->items[i] = (Position){
      .instrument  = seed[i].inst,
      .cusip       = seed[i].cusip,
      .asset_class = seed[i].ac,
      .book        = seed[i].book,
      .desk        = seed[i].desk,
      .notional    = seed[i].notl,
      .avg_price   = seed[i].avg,
      .mkt_price   = seed[i].mkt,
      .pnl_total   = seed[i].pnl,
      .pnl_day     = seed[i].pnl_d,
      .dv01        = seed[i].dv01,
      .cs01        = seed[i].cs01,
      .delta       = seed[i].delta,
      .gamma       = seed[i].gamma,
      .vega        = seed[i].vega,
      .theta       = seed[i].theta,
      .stale       = seed[i].stale,
    };
  }
}


void data_simulate_tick(PositionBook *book, int tick) {
  if (tick % 15 != 0) return;  /* update every ~15 frames (~250ms at 60fps) */

  for (int i = 0; i < book->count; i++) {
    Position *p = &book->items[i];
    double jitter = ((rand() % 2001) - 1000) / 100000.0;
    if (p->mkt_price > 0.01) {
      p->mkt_price += jitter;
      p->pnl_day   += jitter * p->notional * 10.0;
      p->pnl_total += jitter * p->notional * 10.0;
    }
    p->pnl_day   += p->theta * 0.001;
    p->pnl_total += p->theta * 0.001;
  }
}
