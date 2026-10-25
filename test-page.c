#include <assert.h>
#include "sound-rec.c"
void sound_rec_info_hook(void) {}
static int saved, redraws, texts;
static void save(const char *id, const char *key, int value) {
    assert(!strcmp(id, MOD_ID) && !strcmp(key, "record-bits")); saved = value;
}
static void redraw(void) { ++redraws; }
static int width(const char *s, int px) { return (int)strlen(s) * px / 2; }
static int height(az_draw *d) { (void)d; return 48; }
static uint32_t color(int role) { return (uint32_t)role; }
static void text(az_draw *d, int x, int y, const char *s, int px, uint32_t c) {
    (void)d; (void)x; (void)y; (void)s; (void)px; (void)c; ++texts;
}
static void fill(az_draw *d, int x, int y, int w, int h, uint32_t c) {
    (void)d; (void)x; (void)y; (void)c; assert(w > 0 && h > 0);
}
static void rect(az_draw *d, int x, int y, int w, int h, int line, uint32_t c) {
    (void)line; fill(d,x,y,w,h,c);
}
int main(void) {
    az_core_v1 api = {.cfg_set=save, .request_redraw=redraw, .text_width=width,
        .row_height=height, .color=color, .draw_text=text, .fill_rect=fill, .draw_rect=rect};
    C = &api; ready = 1; page_enter(NULL);
    az_event e = {.kind=AZ_EV_TAP,.x=10,.y=110};
    assert(!page_input(&e,NULL)); /* no stale hit targets before paint */
    page_draw(NULL,(az_rect){0,0,500,360},NULL);
    assert(layout_ready && texts == 8);
    e.x=buttons[1].x+2; e.y=buttons[1].y+2;
    assert(page_input(&e,NULL) && saved==24 && sound_rec_requested_bits==24);
    e.x=buttons[0].x+2;
    assert(page_input(&e,NULL) && saved==16 && sound_rec_requested_bits==16);
    e=(az_event){.kind=AZ_EV_KNOB,.dir=1}; page_input(&e,NULL);
    assert(saved==16); /* moving focus alone never changes the preference */
    e.kind=AZ_EV_CLICK; page_input(&e,NULL); assert(saved==24);
    e.kind=AZ_EV_BACK; assert(!page_input(&e,NULL));
    choose_bits(32); assert(saved==24);
    ready=0; choose_bits(16); assert(saved==24);
    page_draw(NULL,(az_rect){0,0,120,60},NULL); assert(!layout_ready);
    assert(redraws==4);
    return 0;
}
