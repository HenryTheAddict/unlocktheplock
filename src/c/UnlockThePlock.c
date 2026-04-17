/*
 * UNLOCK THE PLOCK — Pop the Lock for Pebble
 * Targets: chalk (Round 2, 180x180) · emery (Time 2, 200x228)
 */
#include <pebble.h>

/* ══════════════════════════════════════════════════════════════════
 *  Platform geometry
 * ══════════════════════════════════════════════════════════════════ */
#if defined(PBL_ROUND)
  #define RING_RADIUS  72
#elif defined(PBL_PLATFORM_EMERY)
  #define RING_RADIUS  82
#else
  #define RING_RADIUS  64
#endif

#define RING_THICK   6
#define IND_R        9    /* indicator core radius */
#define IND_GLOW     5    /* glow extra pixels     */
#define FRAME_MS     33   /* ~30 fps               */

/* ══════════════════════════════════════════════════════════════════
 *  Persist
 * ══════════════════════════════════════════════════════════════════ */
#define PK_SCHEMA  99
#define PK_HS       0
#define PK_HHS      1
#define SCHEMA_V    2

/* ══════════════════════════════════════════════════════════════════
 *  Gameplay tuning
 * ══════════════════════════════════════════════════════════════════ */
#define SPD_INIT    320
#define SPD_INC      60
#define SPD_MAX    3000
#define TOL_INIT   3600   /* ~20° window to start       */
#define TOL_MIN    1000   /* ~5.5° window at max diff   */
#define TOL_DEC      85
#define FLASH_DUR    12
#define SHAKE_DUR    16
#define NPARTS       36
#define NSTARS       40
#define PAUSE_CD     60   /* ~2 s at 30 fps             */
#define POP_FRAMES   24   /* ring-pulse animation len   */
#define POP_SPEED     5   /* px/frame ring expansion    */
#define SHINE_FRAMES  8   /* ring gold-flash frames     */
#define TRAIL_LEN     5   /* trail dots behind orb      */

/* ══════════════════════════════════════════════════════════════════
 *  Types
 * ══════════════════════════════════════════════════════════════════ */
typedef enum { ST_TITLE, ST_PLAYING, ST_PAUSED,
               ST_SUCCESS, ST_FAIL, ST_OVER } St;
typedef enum { MODE_CLASSIC, MODE_HARDCORE, MODE_ZEN } Mode;

typedef struct {
  int32_t x, y, vx, vy;   /* fixed-point *256 */
  int life, maxlife, sz;
  GColor col;
} Part;

typedef struct { int32_t x, y, z; } Star;

typedef struct {
  St      st;
  Mode    mode;
  int32_t angle;     /* indicator angle (trig units)    */
  int32_t target;    /* target zone centre               */
  int     spd;       /* angle-units per frame            */
  bool    cw;        /* clockwise?                       */
  int32_t tol;       /* hit tolerance (trig units)       */
  int     level;
  int     hs;        /* classic high score               */
  int     hhs;       /* hardcore high score              */
  bool    new_hs;
  bool    zen_miss;  /* missed in zen mode?              */
  int32_t z_last_ang;/* last miss angle (zen anti-spam)  */
  int     z_spam;    /* spam hit count                   */
  int     flash;     /* corner-flash countdown           */
  GColor  fcol;
  int     shake;     /* screen-shake countdown           */
  int     sx, sy;    /* shake pixel offset               */
  Part    p[NPARTS];
  bool    p_on;
  Star    stars[NSTARS];
  /* ── animation state ── */
  int     pop_r;     /* expanding ring radius            */
  int     pop_t;     /* expanding ring countdown         */
  int     shine_t;   /* ring gold-flash countdown        */
  int     level_pop; /* level-number bounce countdown    */
  int     pause_cd;
  int32_t title_ang;
  int32_t travel;    /* distance traveled in current turn */
  int     trail_f;   /* trail flip interpolation (0-256)  */
  int     frame;
} Game;

/* ══════════════════════════════════════════════════════════════════
 *  Globals
 * ══════════════════════════════════════════════════════════════════ */
static Window   *s_win;
static Layer    *s_cvs;
static AppTimer *s_tmr;
static Game      G;

/* ══════════════════════════════════════════════════════════════════
 *  Math helpers
 * ══════════════════════════════════════════════════════════════════ */
static GPoint ring_pt(int32_t a, int cx, int cy) {
  return GPoint(
    (int16_t)(cx + sin_lookup(a) * RING_RADIUS / TRIG_MAX_RATIO),
    (int16_t)(cy - cos_lookup(a) * RING_RADIUS / TRIG_MAX_RATIO));
}

static GPoint radius_pt(int32_t a, int cx, int cy, int r) {
  return GPoint(
    (int16_t)(cx + sin_lookup(a) * r / TRIG_MAX_RATIO),
    (int16_t)(cy - cos_lookup(a) * r / TRIG_MAX_RATIO));
}

static int32_t ang_dist(int32_t a, int32_t b) {
  int32_t d = ((a - b) % TRIG_MAX_ANGLE + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
  return d > TRIG_MAX_ANGLE / 2 ? TRIG_MAX_ANGLE - d : d;
}

static int32_t ang_wrap(int32_t a) {
  return ((a % TRIG_MAX_ANGLE) + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
}

/* ══════════════════════════════════════════════════════════════════
 *  Particles
 * ══════════════════════════════════════════════════════════════════ */
static GColor pcolor(int i) {
#ifdef PBL_COLOR
  switch (i % 7) {
    case 0: return GColorYellow;
    case 1: return GColorOrange;
    case 2: return GColorGreen;
    case 3: return GColorCyan;
    case 4: return GColorWhite;
    case 5: return GColorSunsetOrange;
    default: return GColorMintGreen;
  }
#else
  return GColorWhite;
#endif
}

static void parts_emit(int cx, int cy) {
  for (int i = 0; i < NPARTS; i++) {
    int32_t a = (TRIG_MAX_ANGLE * i) / NPARTS + (rand() % 3000 - 1500);
    int spd   = 4 + (rand() % 8);
    G.p[i].x       = cx * 256;
    G.p[i].y       = cy * 256;
    G.p[i].vx      = sin_lookup(a) * spd >> 7;
    G.p[i].vy      = -(cos_lookup(a) * spd >> 7);
    G.p[i].life    = 44 + (rand() % 28);
    G.p[i].maxlife = 72;
    G.p[i].sz      = 2 + (rand() % 4);
    G.p[i].col     = pcolor(i);
  }
  G.p_on = true;
}

static void parts_update(void) {
  bool any = false;
  for (int i = 0; i < NPARTS; i++) {
    if (G.p[i].life <= 0) continue;
    G.p[i].x  += G.p[i].vx;
    G.p[i].y  += G.p[i].vy;
    G.p[i].vx  = (G.p[i].vx * 248) >> 8; /* air resistance */
    G.p[i].vy  = (G.p[i].vy * 248) >> 8;
    G.p[i].vy += 20; /* gravity */
    G.p[i].life--;

    /* fade out color if possible */
#ifdef PBL_COLOR
    if (G.p[i].life < 24) {
      // Shift towards black/darker shades as they die
      if (G.p[i].life < 8) G.p[i].col = GColorBlack;
      else if (G.p[i].life < 16) G.p[i].col = GColorDarkGray;
      else if (G.p[i].life < 24) G.p[i].col = GColorLightGray;
    }
#endif
    any = true;
  }
  G.p_on = any;
}

static void parts_draw(GContext *ctx) {
  for (int i = 0; i < NPARTS; i++) {
    if (G.p[i].life <= 0) continue;
    int sz = G.p[i].sz * G.p[i].life / G.p[i].maxlife;
    if (sz < 1) continue; // Shrink to zero and disappear
    graphics_context_set_fill_color(ctx, G.p[i].col);
    graphics_fill_circle(ctx, GPoint(G.p[i].x / 256, G.p[i].y / 256), sz);
  }
}

/* ══════════════════════════════════════════════════════════════════
 *  Starfield
 * ══════════════════════════════════════════════════════════════════ */
static void stars_init(int w, int h) {
  for (int i = 0; i < NSTARS; i++) {
    G.stars[i].x = (rand() % w - w / 2) << 8;
    G.stars[i].y = (rand() % h - h / 2) << 8;
    G.stars[i].z = (rand() % 256) + 1;
  }
}

static void stars_draw(GContext *ctx, int w, int h) {
  int cx  = w / 2;
  int cy  = h / 2;
#ifndef PBL_COLOR
  (void)ctx; (void)cx; (void)cy; (void)w; (void)h;
  return; /* Simplified B&W: No background starfield */
#endif

  int spd = (G.st == ST_PLAYING) ? (G.spd / 30 + 4) : 2;

  for (int i = 0; i < NSTARS; i++) {
    int32_t last_z = G.stars[i].z;
    G.stars[i].z -= spd;
    if (G.stars[i].z <= 0) {
      G.stars[i].z = 256;
      G.stars[i].x = (rand() % w - w / 2) << 8;
      G.stars[i].y = (rand() % h - h / 2) << 8;
      last_z = 256;
    }

    int x1 = cx + (G.stars[i].x / G.stars[i].z);
    int y1 = cy + (G.stars[i].y / G.stars[i].z);
    int x2 = cx + (G.stars[i].x / last_z);
    int y2 = cy + (G.stars[i].y / last_z);

    if (x1 < 0 || x1 >= w || y1 < 0 || y1 >= h) continue;

#ifdef PBL_COLOR
    GColor col = (G.stars[i].z < 80)  ? GColorWhite
               : (G.stars[i].z < 150) ? GColorPictonBlue
               : (G.stars[i].z < 200) ? GColorCobaltBlue
               :                        GColorDarkGray;
#else
    GColor col = GColorWhite;
#endif

    graphics_context_set_stroke_color(ctx, col);
    graphics_context_set_stroke_width(ctx, (G.stars[i].z < 100) ? 2 : 1);
    graphics_draw_line(ctx, GPoint(x1, y1), GPoint(x2, y2));
  }
}

/* ══════════════════════════════════════════════════════════════════
 *  Game logic
 * ══════════════════════════════════════════════════════════════════ */
static void g_reset(void) {
  G.st        = ST_TITLE;
  G.title_ang = 0;
  G.frame     = 0;
  G.p_on      = false;
  G.flash     = 0;
  G.shake     = 0;
  G.sx = G.sy = 0;
  G.level_pop = 0;
  G.pause_cd  = 0;
  G.trail_f   = 256;
  G.zen_miss  = false;
  G.z_spam    = 0;
  G.pop_t     = 0;
  G.shine_t   = 0;
}

static void g_start(void) {
  G.st      = ST_PLAYING;
  G.level   = 1;
  G.angle   = 0;
  G.spd     = (G.mode == MODE_HARDCORE) ? (SPD_INIT * 3 / 2) : SPD_INIT;
  G.tol     = (G.mode == MODE_HARDCORE) ? (TOL_INIT * 4 / 5) : TOL_INIT;
  G.cw      = true;
  G.new_hs  = false;
  G.zen_miss = false;
  G.z_spam    = 0;
  G.flash   = 0;
  G.shake   = 0;
  G.sx = G.sy = 0;
  G.p_on    = false;
  G.level_pop = 0;
  G.pause_cd  = 0;
  G.trail_f   = 256;
  G.pop_t   = 0;
  G.shine_t = 0;
  G.travel  = 0;
  G.target  = (int32_t)(rand() % TRIG_MAX_ANGLE);
}

static void g_next(void) {
  if (G.mode == MODE_ZEN && G.zen_miss) {
    /* missed in Zen: don't increment level or difficulty */
  } else {
    if (G.mode == MODE_HARDCORE) {
      if (G.level > G.hhs) {
        G.hhs = G.level;
        G.new_hs = true;
        persist_write_int(PK_HHS, G.hhs);
      }
    } else if (G.mode == MODE_CLASSIC) {
      if (G.level > G.hs) {
        G.hs = G.level;
        G.new_hs = true;
        persist_write_int(PK_HS, G.hs);
      }
    }
    G.level++;
    int inc = (G.mode == MODE_HARDCORE) ? (SPD_INC * 3 / 2)
            : (G.mode == MODE_ZEN)      ? (SPD_INC / 2)
            :                             SPD_INC;
    G.spd += inc;
    if (G.spd > SPD_MAX) G.spd = SPD_MAX;

    int dec = (G.mode == MODE_HARDCORE) ? (TOL_DEC * 6 / 5)
            : (G.mode == MODE_ZEN)      ? (TOL_DEC / 2)
            :                             TOL_DEC;
    G.tol -= dec;
    if (G.tol < TOL_MIN) G.tol = TOL_MIN;
  }

  G.cw      = !G.cw;
  G.trail_f = 0;
  G.level_pop = 12;
  G.travel  = 0;
  G.target  = (int32_t)(rand() % TRIG_MAX_ANGLE);
}

static void g_select(void) {
  if (G.st == ST_TITLE)  { g_start(); return; }
  if (G.st == ST_PAUSED) { return; }
  if (G.st == ST_OVER)   { g_reset(); return; }
  if (G.st != ST_PLAYING) return;

  int32_t d = ang_dist(G.angle, G.target);

  if (d <= G.tol) {
    /* ── HIT ── */
    G.st      = ST_SUCCESS;
    G.flash   = FLASH_DUR;
    G.zen_miss = false;
    G.z_spam    = 0;
    G.shake   = 6;                  /* small satisfying hit shake */
    G.pop_r   = RING_RADIUS;        /* start expanding ring pulse */
    G.pop_t   = POP_FRAMES;
    G.shine_t = SHINE_FRAMES;       /* ring briefly turns gold    */
#ifdef PBL_COLOR
    G.fcol = GColorYellow;
#else
    G.fcol = GColorWhite;
#endif
    GRect bnd = layer_get_bounds(s_cvs);
    GPoint ip = ring_pt(G.angle, bnd.size.w/2 + G.sx, bnd.size.h/2 + G.sy);
    parts_emit(ip.x, ip.y);
    static const uint32_t segments[] = { 30, 20, 30 };
    VibePattern pat = { .durations = segments, .num_segments = 3 };
    vibes_enqueue_custom_pattern(pat);
    light_enable_interaction();
  } else {
    /* ── MISS ── */
    if (G.mode == MODE_ZEN) {
      /* Anti-spam: check if spamming same area */
      if (ang_dist(G.angle, G.z_last_ang) < TRIG_MAX_ANGLE / 24) {
        if (++G.z_spam > 3) {
          /* Spam detected -> Fail */
          goto zen_fail;
        }
      } else {
        G.z_last_ang = G.angle;
        G.z_spam     = 1;
      }

      /* Zen mode: forgive the miss, keep going */
      G.st    = ST_SUCCESS;
      G.flash = FLASH_DUR;
      G.zen_miss = true;
      G.pop_r = RING_RADIUS;
      G.pop_t = POP_FRAMES / 2;
#ifdef PBL_COLOR
      G.fcol = GColorOrange;
#else
      G.fcol = GColorWhite;
#endif
      vibes_double_pulse();
      return;
    }
    zen_fail:
    G.st    = ST_FAIL;
    G.flash = FLASH_DUR * 2;
    G.shake = SHAKE_DUR;
    G.pop_r = RING_RADIUS;
    G.pop_t = POP_FRAMES / 2;       /* faster, smaller red pulse  */
#ifdef PBL_COLOR
    G.fcol = GColorRed;
#else
    G.fcol = GColorWhite;
#endif
    if (G.mode == MODE_HARDCORE) {
      if (G.level > G.hhs) { G.hhs = G.level; G.new_hs = true;
                              persist_write_int(PK_HHS, G.hhs); }
    } else {
      if (G.level > G.hs)  { G.hs  = G.level; G.new_hs = true;
                              persist_write_int(PK_HS,  G.hs);  }
    }
    static const uint32_t segs[] = {80, 50, 160};
    VibePattern vp = {.durations = segs, .num_segments = 3};
    vibes_enqueue_custom_pattern(vp);
  }
}

/* ══════════════════════════════════════════════════════════════════
 *  Lock icon helper — drawn at target point on the ring
 * ══════════════════════════════════════════════════════════════════ */
static void draw_lock(GContext *ctx, GPoint p, bool open, GColor col) {
  graphics_context_set_stroke_color(ctx, col);
  graphics_context_set_stroke_width(ctx, 1);
  /* body */
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, GRect(p.x-4, p.y, 9, 7), 1, GCornersAll);
  graphics_draw_round_rect(ctx, GRect(p.x-4, p.y, 9, 7), 1);
  /* shackle */
  if (!open) {
    /* closed: U sits flush on body */
    graphics_draw_line(ctx, GPoint(p.x-2, p.y),   GPoint(p.x-2, p.y-5));
    graphics_draw_line(ctx, GPoint(p.x+3, p.y),   GPoint(p.x+3, p.y-5));
    graphics_draw_arc(ctx, GRect(p.x-2, p.y-7, 6, 6), GOvalScaleModeFitCircle,
                      TRIG_MAX_ANGLE * 3 / 4, TRIG_MAX_ANGLE * 5 / 4);
  } else {
    /* open: shackle raised and rotated slightly */
    graphics_draw_line(ctx, GPoint(p.x-2, p.y-2),   GPoint(p.x-2, p.y-7));
    graphics_draw_arc(ctx, GRect(p.x-2, p.y-9, 6, 6), GOvalScaleModeFitCircle,
                      TRIG_MAX_ANGLE * 3 / 4, TRIG_MAX_ANGLE * 5 / 4);
    graphics_draw_line(ctx, GPoint(p.x+4, p.y-7),   GPoint(p.x+6, p.y-4));
  }
  /* keyhole */
  graphics_context_set_fill_color(ctx, col);
  graphics_fill_rect(ctx, GRect(p.x, p.y+2, 2, 3), 0, GCornerNone);
  graphics_fill_circle(ctx, GPoint(p.x+1, p.y+2), 1);
}

/* ══════════════════════════════════════════════════════════════════
 *  Drawing
 * ══════════════════════════════════════════════════════════════════ */
static void draw_cb(Layer *layer, GContext *ctx) {
  graphics_context_set_antialiased(ctx, true);
  GRect b  = layer_get_bounds(layer);
  int W    = b.size.w;
  int H    = b.size.h;
  int cx   = W / 2 + G.sx;
  int cy   = H / 2 + G.sy;
  int R    = RING_RADIUS;

  /* ── black background ── */
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  /* ── starfield ── */
  stars_draw(ctx, W, H);


  /* ── radial speed lines (gameplay only) ── */
#ifdef PBL_COLOR
  if (G.st == ST_PLAYING || G.st == ST_PAUSED) {
    int32_t step   = TRIG_MAX_ANGLE / 8;
    int32_t offset = ang_wrap((int32_t)G.frame * G.spd / 400);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_context_set_stroke_color(ctx, GColorDarkGray);
    for (int i = 0; i < 8; i++) {
      int32_t a = ang_wrap(offset + i * step);
      graphics_draw_line(ctx,
        ring_pt(a, cx, cy),
        radius_pt(a, cx, cy, R + 38));
    }
  }
#endif

  /* ════════════════════════════════════════════════════════════
   *  TITLE SCREEN
   * ════════════════════════════════════════════════════════════ */
  if (G.st == ST_TITLE) {
    /* main ring */
    graphics_context_set_stroke_color(ctx, GColorLightGray);
    graphics_context_set_stroke_width(ctx, RING_THICK);
    graphics_draw_circle(ctx, GPoint(cx, cy), R);

    /* orbiting indicator + trail */
    GPoint ip = ring_pt(G.title_ang, cx, cy);
#ifdef PBL_COLOR
    for (int t = TRAIL_LEN; t >= 1; t--) {
      int32_t ta = ang_wrap(G.title_ang - t * (TRIG_MAX_ANGLE / 46));
      GPoint tp2 = ring_pt(ta, cx, cy);
      static const int tsz[5] = {4,4,4,3,2};
      GColor tc = (t > 3) ? GColorDarkCandyAppleRed
                : (t > 1) ? GColorOrange : GColorChromeYellow;
      graphics_context_set_fill_color(ctx, tc);
      graphics_fill_circle(ctx, tp2, tsz[t-1]);
    }
    graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
    graphics_fill_circle(ctx, ip, IND_R + IND_GLOW + 2);
    graphics_context_set_fill_color(ctx, GColorOrange);
    graphics_fill_circle(ctx, ip, IND_R + IND_GLOW);
    graphics_context_set_fill_color(ctx, GColorYellow);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_circle(ctx, ip, IND_R);

    /* mode-specific color */
    GColor mode_col = GColorIcterine;
#ifdef PBL_COLOR
    if (G.mode == MODE_HARDCORE) mode_col = GColorImperialPurple;
    if (G.mode == MODE_ZEN)      mode_col = GColorMediumSpringGreen;
#endif

    /* title */
    GFont tf = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
    GFont sf = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
#ifdef PBL_ROUND
    int ty = cy - 66;
#else
    int ty = cy - 80;
#endif
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, mode_col);
#else
    graphics_context_set_text_color(ctx, GColorWhite);
#endif
    graphics_draw_text(ctx, "UNLOCK", tf, GRect(cx-70, ty, 140, 30),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "THE PLOCK", tf, GRect(cx-80, ty+26, 160, 30),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);

    /* pulsing instruction */
    if ((G.frame % 50) < 38) {
      GFont ifont = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
#ifdef PBL_COLOR
      graphics_context_set_text_color(ctx, GColorElectricBlue);
#else
      graphics_context_set_text_color(ctx, GColorWhite);
#endif
      graphics_draw_text(ctx, "PRESS SELECT", ifont,
        GRect(cx-70, cy-14, 140, 30),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }

    /* mode label + nav hint */
    const char *modes[] = {"CLASSIC", "HARDCORE", "ZEN"};

    /* Selection highlight bar */
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(cx-40, cy+29, 80, 15), 2, GCornersAll);

#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx, mode_col);
#else
    graphics_context_set_text_color(ctx, GColorBlack);
#endif
    graphics_draw_text(ctx, modes[G.mode], sf,
      GRect(cx-50, cy+27, 100, 18),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);

    /* best score */
    int hs = (G.mode == MODE_HARDCORE) ? G.hhs : G.hs;
    {
      static char hsbuf[20];
      if (hs > 0) snprintf(hsbuf, sizeof(hsbuf), "BEST: %d", hs);
      else        snprintf(hsbuf, sizeof(hsbuf), "BEST: --");
      graphics_context_set_text_color(ctx, GColorLightGray);
      graphics_draw_text(ctx, hsbuf, sf,
        GRect(cx-40, cy+46, 80, 18),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
    return;
  }

  /* ════════════════════════════════════════════════════════════
   *  GAMEPLAY + OVER + PAUSED
   * ════════════════════════════════════════════════════════════ */
  int32_t ind = G.angle;
  int32_t tgt = G.target;
  int32_t tol = G.tol;
  int32_t dst = ang_dist(ind, tgt);
  int32_t a_s = tgt - tol;
  int32_t a_e = tgt + tol;

  /* proximity 0–256 */
  int prox = (dst < tol * 2) ? (int)(256 - dst * 256 / (tol * 2)) : 0;

  /* ── derived colours ── */
#ifdef PBL_COLOR
  int spd_range = SPD_MAX - SPD_INIT;
  int spd_pct = (spd_range > 0)
    ? ((G.spd - SPD_INIT) * 100 / spd_range) : 0;
  if (spd_pct < 0)   spd_pct = 0;
  if (spd_pct > 100) spd_pct = 100;
  GColor ring_base = (spd_pct < 25) ? GColorMediumSpringGreen
                   : (spd_pct < 50) ? GColorChromeYellow
                   : (spd_pct < 75) ? GColorOrange
                   :                  GColorRed;
  bool shining    = (G.shine_t > 0);
  GColor ring_col = (prox > 200 || shining) ? GColorWhite : ring_base;
  GColor nc       = (G.st == ST_SUCCESS && G.flash > FLASH_DUR / 2)
                    ? GColorYellow : GColorWhite;
#else
  GColor ring_col = GColorWhite;
  GColor nc       = GColorWhite;
  (void)prox;
#endif

  /* ── expanding ring pulse (hit OR miss) ── */
  if (G.pop_t > 0) {
#ifdef PBL_COLOR
    GColor pc = (G.fcol.argb == GColorRedARGB8)
      ? (G.pop_t > POP_FRAMES/4 ? GColorRed : GColorDarkCandyAppleRed)
      : (G.pop_t > POP_FRAMES/2 ? GColorYellow : GColorChromeYellow);
    graphics_context_set_stroke_color(ctx, pc);
#else
    graphics_context_set_stroke_color(ctx, GColorWhite);
#endif
    int pw = (G.pop_t > POP_FRAMES / 2) ? 4 : 2;
    graphics_context_set_stroke_width(ctx, pw);
    graphics_draw_circle(ctx, GPoint(cx, cy), G.pop_r);
    /* second ring trailing behind */
    if (G.pop_r > R + 8) {
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_circle(ctx, GPoint(cx, cy), G.pop_r - 8);
    }
  }

  /* ── target zone — multi-layer glow arc ── */
#ifdef PBL_COLOR
  int Rg = R + 8;
  graphics_context_set_fill_color(ctx, GColorDarkGreen);
  graphics_fill_radial(ctx, GRect(cx-Rg, cy-Rg, Rg*2, Rg*2),
    GOvalScaleModeFitCircle, RING_THICK + 12, a_s, a_e);
  int Rm = R + 4;
  graphics_context_set_fill_color(ctx, GColorMediumSpringGreen);
  graphics_fill_radial(ctx, GRect(cx-Rm, cy-Rm, Rm*2, Rm*2),
    GOvalScaleModeFitCircle, RING_THICK + 6, a_s, a_e);
  GColor tgt_col = (G.st == ST_SUCCESS) ? GColorYellow : GColorGreen;
  graphics_context_set_fill_color(ctx, tgt_col);
  graphics_fill_radial(ctx, GRect(cx-R, cy-R, R*2, R*2),
    GOvalScaleModeFitCircle, RING_THICK + 2, a_s, a_e);
#else
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, GRect(cx-R-2, cy-R-2, (R+2)*2, (R+2)*2),
    GOvalScaleModeFitCircle, RING_THICK + 4, a_s, a_e);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_radial(ctx, GRect(cx-R, cy-R, R*2, R*2),
    GOvalScaleModeFitCircle, RING_THICK, a_s, a_e);
#endif

  /* ── main ring (drawn over target zone edges) ── */
  graphics_context_set_stroke_color(ctx, ring_col);
  graphics_context_set_stroke_width(ctx, RING_THICK);
  graphics_draw_circle(ctx, GPoint(cx, cy), R);

  /* ── lock icon at target centre ── */
  {
    GPoint tp = ring_pt(tgt, cx, cy);
    bool open  = (G.st == ST_SUCCESS);
#ifdef PBL_COLOR
    GColor lk = open ? GColorYellow : GColorMintGreen;
#else
    GColor lk = GColorWhite;
#endif
    draw_lock(ctx, tp, open, lk);
  }

  /* ── indicator trail ── */
#ifdef PBL_COLOR
  {
    int dir = G.cw ? 1 : -1;
    /* smooth interpolation for trail direction change */
    int32_t flip_off = (256 - G.trail_f) * (TRIG_MAX_ANGLE / 24) >> 8;
    static const int tsz[TRAIL_LEN] = {4,4,4,3,2};
    for (int t = TRAIL_LEN; t >= 1; t--) {
      int32_t ta  = ang_wrap(ind - dir * t * (TRIG_MAX_ANGLE / 46) + (dir * flip_off));
      GPoint  tp2 = ring_pt(ta, cx, cy);
      GColor tc;
      switch (t) {
        case 5: tc = GColorDarkCandyAppleRed; break;
        case 4: tc = GColorBulgarianRose;     break;
        case 3: tc = GColorOrange;            break;
        case 2: tc = GColorChromeYellow;      break;
        default: tc = GColorPastelYellow;     break;
      }
      graphics_context_set_fill_color(ctx, tc);
      graphics_fill_circle(ctx, tp2, tsz[t - 1]);
    }
  }
#endif

  /* ── indicator orb — blink on fail ── */
  bool blink = (G.st == ST_FAIL) && ((G.flash / 3) % 2 == 0);
  if (!blink) {
    GPoint ip = ring_pt(ind, cx, cy);
#ifdef PBL_COLOR
    GColor gc = (prox > 200) ? GColorYellow : GColorDarkCandyAppleRed;
    graphics_context_set_fill_color(ctx, gc);
    graphics_fill_circle(ctx, ip, IND_R + IND_GLOW + 2);
    graphics_context_set_fill_color(ctx, GColorOrange);
    graphics_fill_circle(ctx, ip, IND_R + IND_GLOW);
    GColor cc = (G.st == ST_SUCCESS) ? GColorYellow
              : (prox > 200)         ? GColorYellow
              : (prox > 100)         ? GColorChromeYellow
              :                        GColorOrange;
    graphics_context_set_fill_color(ctx, cc);
#else
    graphics_context_set_fill_color(ctx, GColorWhite);
#endif
    graphics_fill_circle(ctx, ip, IND_R);
  }

  /* ── corner flash accents ── */
  if (G.flash > 0) {
#ifdef PBL_COLOR
    graphics_context_set_stroke_color(ctx, G.fcol);
    int sw = (G.flash > FLASH_DUR / 2) ? 5 : 3;
    graphics_context_set_stroke_width(ctx, sw);
    int m = 10, L = 20;
    graphics_draw_line(ctx, GPoint(m,   m),   GPoint(m+L, m));
    graphics_draw_line(ctx, GPoint(m,   m),   GPoint(m,   m+L));
    graphics_draw_line(ctx, GPoint(W-m, m),   GPoint(W-m-L, m));
    graphics_draw_line(ctx, GPoint(W-m, m),   GPoint(W-m,   m+L));
    graphics_draw_line(ctx, GPoint(m,   H-m), GPoint(m+L,   H-m));
    graphics_draw_line(ctx, GPoint(m,   H-m), GPoint(m,     H-m-L));
    graphics_draw_line(ctx, GPoint(W-m, H-m), GPoint(W-m-L, H-m));
    graphics_draw_line(ctx, GPoint(W-m, H-m), GPoint(W-m,   H-m-L));
#endif
  }

  /* ── particles ── */
  if (G.p_on) parts_draw(ctx);

  /* ── "POP!" flash text on success ── */
#ifdef PBL_COLOR
  if (G.st == ST_SUCCESS && G.flash > FLASH_DUR / 2) {
    GFont pf = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
    graphics_context_set_text_color(ctx, GColorYellow);
    graphics_draw_text(ctx, "POP!", pf,
      GRect(cx-35, cy-52, 70, 24),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
#endif

  /* ── centre HUD ── */
  GFont bf = fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS);
  GFont tf = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  static char lvl_buf[8];
  snprintf(lvl_buf, sizeof(lvl_buf), "%d", G.level);


  int ty_off = G.level_pop;
  graphics_context_set_text_color(ctx, nc);
#ifdef PBL_ROUND
  int level_val_y = cy - 28;
#else
  int level_val_y = cy - 32;
#endif
  graphics_draw_text(ctx, lvl_buf, bf,
    GRect(cx-50, level_val_y-ty_off/2, 100, 64+ty_off),
    GTextOverflowModeFill, GTextAlignmentCenter, NULL);

  graphics_context_set_text_color(ctx, GColorLightGray);
  graphics_draw_text(ctx, G.cw ? ">" : "<", tf,
    GRect(cx+20, cy-9, 16, 18),
    GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  /* ── paused overlay ── */
  if (G.st == ST_PAUSED) {
    /* Full-width inverted band */
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, cy-32, W, 64), 0, GCornerNone);

    graphics_context_set_text_color(ctx, GColorBlack);
    if (G.pause_cd > 0) {
      static char cdbuf[16];
      snprintf(cdbuf, sizeof(cdbuf), "%d", G.pause_cd / 30 + 1);
      graphics_draw_text(ctx, cdbuf, bf,
        GRect(cx-50, cy-28, 100, 64),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    } else {
      graphics_draw_text(ctx, "PAUSED", bf,
        GRect(cx-50, cy-28, 100, 64),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);

      graphics_draw_text(ctx, "back to resume", tf,
        GRect(cx-60, cy+14, 120, 18),
        GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    }
  }

  /* ── game over overlay ── */
  if (G.st == ST_OVER) {
    GFont sf = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);
#ifdef PBL_COLOR
    graphics_context_set_text_color(ctx,
      G.new_hs ? GColorYellow : GColorRed);
    graphics_draw_text(ctx, G.new_hs ? "NEW BEST!" : "MISSED!", sf,
      GRect(cx-50, cy+30, 100, 24),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
#else
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, "MISSED", sf,
      GRect(cx-45, cy+30, 90, 24),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
#endif
    int hs = (G.mode == MODE_HARDCORE) ? G.hhs : G.hs;
    static char hs_buf[20];
    snprintf(hs_buf, sizeof(hs_buf), "BEST: %d", hs);
    graphics_context_set_text_color(ctx, GColorLightGray);
    graphics_draw_text(ctx, hs_buf, tf,
      GRect(cx-40, cy+54, 80, 18),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
    graphics_context_set_text_color(ctx, GColorDarkGray);
    graphics_draw_text(ctx, "tap to play", tf,
      GRect(cx-40, cy+70, 80, 18),
      GTextOverflowModeFill, GTextAlignmentCenter, NULL);
  }
}

/* ══════════════════════════════════════════════════════════════════
 *  Timer / game loop
 * ══════════════════════════════════════════════════════════════════ */
static void tick(void *unused) {
  G.frame++;

  /* shake decay */
  if (G.shake > 0) {
    G.shake--;
    int amp = G.shake / 2 + 1;
    G.sx = (rand() % (amp * 2 + 1)) - amp;
    G.sy = (rand() % (amp * 2 + 1)) - amp;
    if (G.shake == 0) { G.sx = G.sy = 0; }
  }

  /* ring pulse expansion */
  if (G.pop_t > 0) {
    G.pop_r += POP_SPEED;
    G.pop_t--;
  }

  /* ring shine decay */
  if (G.shine_t > 0) G.shine_t--;

  /* level-pop bounce decay */
  if (G.level_pop > 0) G.level_pop--;

  /* trail flip interpolation */
  if (G.trail_f < 256) G.trail_f += 32;
  if (G.trail_f > 256) G.trail_f = 256;

  /* pause cooldown */
  if (G.pause_cd > 0) G.pause_cd--;

  switch (G.st) {
    case ST_TITLE:
      G.title_ang = ang_wrap(G.title_ang + 200);
      break;

    case ST_PAUSED:
      break;

    case ST_PLAYING:
      G.angle = ang_wrap(G.angle + (G.cw ? G.spd : -G.spd));
      G.travel += G.spd;
      if (G.mode != MODE_ZEN && G.travel > TRIG_MAX_ANGLE * 2) {
        /* Failed to hit target within 2 full loops */
        G.st    = ST_FAIL;
        G.flash = FLASH_DUR * 2;
        G.shake = SHAKE_DUR;
#ifdef PBL_COLOR
        G.fcol = GColorRed;
#else
        G.fcol = GColorWhite;
#endif
        vibes_double_pulse();
      }
      break;

    case ST_SUCCESS:
      parts_update();
      if (--G.flash <= 0) {
        G.new_hs = false;
        g_next();
        G.st = ST_PLAYING;
      }
      break;

    case ST_FAIL:
      parts_update();
      if (--G.flash <= 0) G.st = ST_OVER;
      break;

    case ST_OVER:
      break;
  }

  layer_mark_dirty(s_cvs);
  s_tmr = app_timer_register(FRAME_MS, tick, NULL);
}

/* ══════════════════════════════════════════════════════════════════
 *  Input
 * ══════════════════════════════════════════════════════════════════ */
static void btn_any(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  g_select();
}

static void btn_up(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  if (G.st == ST_TITLE) { G.mode = (G.mode + 2) % 3; layer_mark_dirty(s_cvs); }
  else g_select();
}

static void btn_down(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  if (G.st == ST_TITLE) { G.mode = (G.mode + 1) % 3; layer_mark_dirty(s_cvs); }
  else g_select();
}

static void btn_back(ClickRecognizerRef r, void *ctx) {
  (void)r; (void)ctx;
  if (G.st == ST_PLAYING) {
    G.st = ST_PAUSED; G.pause_cd = PAUSE_CD;
  } else if (G.st == ST_PAUSED && G.pause_cd == 0) {
    G.st = ST_PLAYING;
  } else if (G.st == ST_TITLE) {
    window_stack_pop_all(true);
  } else if (G.st == ST_OVER) {
    g_reset();
  }
}

static void click_config(void *ctx) {
  (void)ctx;
  window_single_click_subscribe(BUTTON_ID_SELECT, btn_any);
  window_single_click_subscribe(BUTTON_ID_UP,     btn_up);
  window_single_click_subscribe(BUTTON_ID_DOWN,   btn_down);
  window_single_click_subscribe(BUTTON_ID_BACK,   btn_back);
}

/* ══════════════════════════════════════════════════════════════════
 *  Window lifecycle
 * ══════════════════════════════════════════════════════════════════ */
static void win_load(Window *win) {
  Layer *root = window_get_root_layer(win);
  GRect  bnd  = layer_get_bounds(root);
  stars_init(bnd.size.w, bnd.size.h);
  s_cvs = layer_create(bnd);
  layer_set_update_proc(s_cvs, draw_cb);
  layer_add_child(root, s_cvs);
  s_tmr = app_timer_register(FRAME_MS, tick, NULL);
}

static void win_unload(Window *win) {
  (void)win;
  if (s_tmr) { app_timer_cancel(s_tmr); s_tmr = NULL; }
  layer_destroy(s_cvs);
}

/* ══════════════════════════════════════════════════════════════════
 *  App init / deinit
 * ══════════════════════════════════════════════════════════════════ */
static void init(void) {
  if (persist_read_int(PK_SCHEMA) != SCHEMA_V) {
    persist_delete(PK_HS);
    persist_delete(PK_HHS);
    persist_write_int(PK_SCHEMA, SCHEMA_V);
  }
  G.hs  = persist_exists(PK_HS)  ? persist_read_int(PK_HS)  : 0;
  G.hhs = persist_exists(PK_HHS) ? persist_read_int(PK_HHS) : 0;
  G.mode = MODE_CLASSIC;
  g_reset();

  s_win = window_create();
  window_set_background_color(s_win, GColorBlack);
  window_set_window_handlers(s_win, (WindowHandlers){
    .load = win_load, .unload = win_unload,
  });
  window_set_click_config_provider(s_win, click_config);
  window_stack_push(s_win, true);
}

static void deinit(void) { window_destroy(s_win); }

int main(void) { init(); app_event_loop(); deinit(); }
