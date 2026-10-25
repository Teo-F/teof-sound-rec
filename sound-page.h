/* Community Mods settings page; all callbacks run on the UI thread. */
static const az_core_v1 *C;
static az_rect buttons[2];
static int layout_ready;
static int selected;

static int page_api_ok(const az_core_v1 *api)
{
    return AZ_HAS(api, cfg_get) && AZ_HAS(api, cfg_set) &&
        AZ_HAS(api, register_screen) && AZ_HAS(api, draw_text) &&
        AZ_HAS(api, fill_rect) && AZ_HAS(api, draw_rect) &&
        AZ_HAS(api, color) && AZ_HAS(api, row_height) &&
        AZ_HAS(api, text_width) && AZ_HAS(api, request_redraw);
}

static void choose_bits(int bits)
{
    if (!ready || (bits != 16 && bits != 24)) return;
    atomic_store_explicit(&sound_rec_requested_bits, bits, memory_order_release);
    C->cfg_set(MOD_ID, "record-bits", bits);
    C->request_redraw();
}

static void page_text(az_draw *d, az_rect area, int y, const char *text, int px, uint32_t color)
{
    while (px > 8 && C->text_width(text, px) > area.w) --px;
    C->draw_text(d, area.x, y, text, px, color);
}

static void page_draw(az_draw *d, az_rect area, void *user)
{
    (void)user;
    layout_ready = 0;
    int rh = C->row_height(d), gap = 8;
    if (rh < 24) rh = 24;
    int px = rh / 3;
    if (px < 8) px = 8;
    uint32_t fg = C->color(AZ_COL_TEXT), dim = C->color(AZ_COL_TEXT_DIM);
    page_text(d, area, area.y, "Sound / Recording", px + 4, C->color(AZ_COL_HI));
    if (area.h < rh * 3 || area.w < 100) return;
    page_text(d, area, area.y + rh, "Next REC bit depth", px, fg);
    int bw = (area.w - gap) / 2, y = area.y + rh * 2;
    unsigned bits = atomic_load_explicit(&sound_rec_requested_bits, memory_order_acquire);
    for (int i = 0; i < 2; ++i) {
        buttons[i] = (az_rect){area.x + i * (bw + gap), y, bw, rh};
        az_rect b = buttons[i];
        int lit = bits == (unsigned)(i ? 24 : 16);
        uint32_t bg = C->color(lit ? AZ_COL_HI : AZ_COL_BG);
        uint32_t ink = C->color(lit ? AZ_COL_HI_TEXT : AZ_COL_TEXT);
        C->fill_rect(d, b.x, b.y, b.w, b.h, bg);
        C->draw_rect(d, b.x, b.y, b.w, b.h, selected == i ? 3 : 1, fg);
        const char *label = i ? "24-bit" : "16-bit";
        C->draw_text(d, b.x + (b.w - C->text_width(label, px)) / 2,
                     b.y + (rh - px * 3 / 2) / 2, label, px, ink);
    }
    layout_ready = 1;
    const char *lines[] = {"Sample rate: 44.1 kHz", "Format: stereo WAV (PCM)",
        "Applies to the next recording.", "Selected preference; recording file not verified."};
    for (int i = 0; i < 4; ++i) {
        y = area.y + rh * (3 + i);
        if (y + rh > area.y + area.h) break;
        page_text(d, area, y, lines[i], px, i < 2 ? fg : dim);
    }
}

static int page_input(const az_event *ev, void *user)
{
    (void)user;
    if (!ev || ev->kind == AZ_EV_BACK) return 0;
    if (ev->kind == AZ_EV_KNOB) {
        selected = ev->dir > 0 ? 1 : 0;
        C->request_redraw();
        return 1;
    }
    if (ev->kind == AZ_EV_CLICK) {
        choose_bits(selected ? 24 : 16);
        return 1;
    }
    if (ev->kind == AZ_EV_TAP && layout_ready) {
        for (int i = 0; i < 2; ++i) {
            az_rect b = buttons[i];
            if (ev->x >= b.x && ev->x < b.x + b.w && ev->y >= b.y && ev->y < b.y + b.h) {
                selected = i;
                choose_bits(i ? 24 : 16);
                return 1;
            }
        }
    }
    return 0;
}

static void page_enter(void *user)
{
    (void)user;
    layout_ready = 0;
    selected = atomic_load_explicit(&sound_rec_requested_bits, memory_order_acquire) == 24;
}

static const az_screen_v1 PAGE = {
    .size = sizeof(az_screen_v1), .on_draw = page_draw,
    .on_input = page_input, .on_enter = page_enter,
};
