// The command header layout is taken from the first command list that walks in one, not from the first list generated.
// Mario Kart 8 Deluxe (SDK 4.4.0): the render thread generated a list before the game's first RequestUpdate whose third command was zeroed; taking the layout from it
// left the SDK 6 headers read in the newer layout, no DeviceSink was found and the game was silent. Also check recovery with the other supported headers and sinks.
// Bastion (SDK 5.4.0): its first list, two commands with the SDK 6 header whose first has ID 0x14 and 0x14 bytes, walks in the SDK 8–20 layout too and must decide nothing.
#include "../hle.h"
#include <assert.h>
#include <sys/wait.h>
#include "../hle_audren.c"

static int acquire_result = 1, acquire_args[4], append_result = 1, append_calls;
static int fake_acquire(void *config, void *voice, int rate, int channels, int format, int priority, const void *extra, size_t extra_sz) {
    (void)config; (void)voice; (void)extra; (void)extra_sz;
    acquire_args[0] = rate; acquire_args[1] = channels; acquire_args[2] = format; acquire_args[3] = priority;
    return acquire_result;
}
static int fake_append(void *voice, const void *wb) { (void)voice; (void)wb; append_calls++; return append_result; }
void *guest_sym(const char *name) {
    if (!strcmp(name, "_ZN2nn5audio16AcquireVoiceSlotEPNS0_19AudioRendererConfigEPNS0_9VoiceTypeEiiNS0_12SampleFormatEiPKvm")) return (void *)(uintptr_t)fake_acquire;
    if (!strcmp(name, "_ZN2nn5audio16AppendWaveBufferEPNS0_9VoiceTypeEPKNS0_10WaveBufferE")) return (void *)(uintptr_t)fake_append;
    return NULL;
}
void *objmap_get(const void *key) { (void)key; return NULL; }
void objmap_put(const void *key, void *value) { (void)key; (void)value; }
void *objmap_del(const void *key) { (void)key; return NULL; }
void *dm_guest_thread_spawn(void (*fn)(void *), void *arg, int prio, size_t stack) { (void)fn; (void)arg; (void)prio; (void)stack; return NULL; }
void dm_guest_thread_join(void *tt) { (void)tt; }
uint64_t now_ns(void) { return 0; }
void *dm_pcmout_open(int ch) { (void)ch; return NULL; }
void dm_pcmout_close(void *k) { (void)k; }
void dm_pcmout_start(void *k) { (void)k; }
int dm_pcmout_append(void *k, void *b) { (void)k; (void)b; return 0; }
void *dm_pcmout_released(void *k) { (void)k; return NULL; }
void dm_pcmout_wait(void *k, int ms) { (void)k; (void)ms; }
void hle_bind_system_event(void *ev, int ac) { (void)ev; (void)ac; }
int hle_signal_bound_system_event(void *ev) { (void)ev; return 0; }
uint32_t hle_dup_readable_handle(uint32_t h) { return h; }

enum { SAMPLES = 240, NMIX = 8, NCMD = 6 };
static int32_t mix[NMIX * SAMPLES];
_Alignas(8) static uint8_t list[0x20 + NCMD * 0x148];
static uint8_t *put(uint8_t *q, int sdk, int id, int size) {
    memset(q, 0, (size_t)size);
    if (sdk == 21) { q[0] = 1; q[1] = (uint8_t)id; *(int16_t *)(q + 2) = (int16_t)size; }
    else {
        *(uint32_t *)q = 0xcafebabeu;
        if (sdk <= 6) { *(int16_t *)(q + 4) = (int16_t)size; *(int16_t *)(q + 6) = (int16_t)id; q[0x10] = 1; }
        else { q[4] = 1; q[5] = (uint8_t)id; *(int16_t *)(q + 6) = (int16_t)size; }
    }
    return q + size;
}
static size_t build(int sdk, int ncmd) {
    memset(list, 0, sizeof list);
    *(uint32_t *)(list + 4) = (uint32_t)ncmd; *(const int32_t **)(list + 8) = mix; *(uint32_t *)(list + 0x14) = SAMPLES; *(uint32_t *)(list + 0x18) = 48000;
    uint8_t *q = list + 0x20;
    for (int i = 0; i < ncmd - 1; i++) q = put(q, sdk, 6, 0x40);
    int id = sdk <= 5 ? 0xf : sdk == 6 ? 0x10 : sdk == 21 ? 0x14 : 0x13;
    uint8_t *sink = q; q = put(q, sdk, id, sdk == 4 ? 0x130 : sdk == 5 ? 0x138 : sdk == 21 ? 0x140 : 0x148);
    int count = sdk <= 5 ? 0x120 : sdk == 21 ? 0x128 : 0x130;
    *(uint32_t *)(sink + count) = 2;
    if (sdk == 4) { sink[0x124] = 0; sink[0x125] = 1; }
    else { *(int16_t *)(sink + count + 4) = 0; *(int16_t *)(sink + count + 6) = 1; }
    return (size_t)(q - list);
}

static void test_headers(void) {
    const int sdks[] = {4, 5, 6, 16, 21};
    for (size_t i = 0; i < sizeof sdks / sizeof *sdks; i++) {
        _Alignas(8) uint8_t aligned[0x40], wire[0x41]; uint8_t *q = wire + 1;
        int sdk = sdks[i], id = sdk == 21 ? 0x14 : 0x10;
        g_hdr6 = sdk <= 6; g_hdr21 = sdk == 21;
        put(aligned, sdk, id, sizeof aligned); memcpy(q, aligned, sizeof aligned);
        assert(cmd_size(q) == sizeof aligned && cmd_id(q) == id && cmd_on(q));
        assert(cmd_ok(q, q + sizeof aligned));
        // Header and body truncation must fail before any field or next-command read.
        for (size_t n = 0; n < sizeof aligned; n++) assert(!cmd_ok(q, q + n));
        const int16_t invalid[] = {0, -1, 4, 0x41};
        for (size_t n = 0; n < sizeof invalid / sizeof *invalid; n++) {
            memcpy(q + (sdk == 21 ? 2 : sdk <= 6 ? 4 : 6), invalid + n, sizeof *invalid);
            assert(!cmd_ok(q, q + sizeof aligned));
        }
    }
    g_hdr6 = g_hdr21 = 0;
    int16_t out[FRAME_MAX * 2];
    for (size_t n = 0; n < 0x20; n++) assert(capture(list, n, out) == 0);
}

static void test(int sdk, int incomplete) {
    // render_once starts with ID 0x14 for SDK 20+ (the Span version of GenerateCommand), 0x13 otherwise.
    g_sink_id = sdk == 21 ? 0x14 : 0x13;
    for (int s = 0; s < SAMPLES; s++) { mix[s] = 1000; mix[SAMPLES + s] = -1000; }
    int16_t out[FRAME_MAX * 2];

    // An empty startup list must not choose a layout either.
    size_t n = build(sdk, NCMD);
    *(uint32_t *)(list + 4) = 0;
    assert(capture(list, 0x20, out) == 0);
    if (incomplete) {
        // The third command is zeroed, as Mario Kart 8 Deluxe's first list was. Several such frames must leave detection open.
        n = build(sdk, NCMD);
        memset(list + 0x20 + 2 * 0x40, 0, 0x40);
        for (int i = 0; i < 3; i++) assert(capture(list, n, out) == 0);
        assert(!g_hdr6 && !g_hdr21 && !g_sink4);
    }
    if (sdk == 5) {
        // Bastion's first list: a 0x14-byte command of ID 0x14, then a 0x28-byte one of ID 0xa: in the SDK 8–20 layout the IDs read as sizes, and both still walk
        memset(list, 0, sizeof list);
        *(uint32_t *)(list + 4) = 2; *(const int32_t **)(list + 8) = mix; *(uint32_t *)(list + 0x14) = SAMPLES; *(uint32_t *)(list + 0x18) = 48000;
        uint8_t *q = put(list + 0x20, 5, 0x14, 0x14); q = put(q, 5, 0xa, 0x28);
        assert(capture(list, (size_t)(q - list), out) == 0);
        assert(!g_hdr6 && !g_hdr21 && !g_sink4);
    }

    // SDK 21 starts with short valid lists: detect its header before the first full list's sink scan.
    n = build(sdk, sdk == 21 ? 2 : NCMD);
    int got = capture(list, n, out);
    assert(g_hdr6 == (sdk <= 6) && g_hdr21 == (sdk == 21) && g_sink4 == (sdk <= 5) && g_sink5 == (sdk == 5));
    assert(g_sink_id == (sdk <= 5 ? 0xf : sdk == 6 ? 0x10 : sdk == 21 ? 0x14 : 0x13));
    assert(got == SAMPLES);
    for (int s = 0; s < SAMPLES; s++) assert(out[2 * s] == 1000 && out[2 * s + 1] == -1000);
    n = build(sdk, NCMD);
    assert(capture(list, n, out) == SAMPLES);   // detection stays valid for later full frames
}

// Malformed lists after layout selection must leave the histogram/sink scan pending
// and must never produce PCM, even when a valid sink precedes the malformed tail.
static void test_malformed(int sdk, int bad) {
    g_sink_id = sdk == 21 ? 0x14 : 0x13;
    int16_t out[FRAME_MAX * 2];
    size_t n = build(sdk, NCMD);
    *(uint32_t *)(list + 4) = 1;
    assert(capture(list, 0x20 + 0x40, out) == 0);   // select the layout without a sink or histogram
    int sink_id = g_sink_id;
    n = build(sdk, NCMD);
    if (bad < 2) memset(list + 0x20 + (size_t)bad * 0x40, 0, 0x40);
    else { *(uint32_t *)(list + 4) = NCMD + 1; memset(list + n, 0, 0x40); n += 0x40; }
    for (size_t i = 0; i < sizeof out / sizeof *out; i++) out[i] = 1234;

    FILE *log = tmpfile(); assert(log);
    fflush(stderr); int saved = dup(STDERR_FILENO); assert(saved >= 0);
    assert(dup2(fileno(log), STDERR_FILENO) >= 0);
    int got = capture(list, n, out);
    fflush(stderr); assert(dup2(saved, STDERR_FILENO) >= 0); close(saved);
    char text[2048]; assert(fseek(log, 0, SEEK_SET) == 0); size_t bytes = fread(text, 1, sizeof text - 1, log); text[bytes] = 0; fclose(log);
    assert(got == 0);
    assert(!strstr(text, "by command ID:"));
    assert(strstr(text, "malformed"));
    assert(g_sink_id == sink_id && !g_sink4 && !g_sink5);
    for (size_t i = 0; i < sizeof out / sizeof *out; i++) assert(out[i] == 1234);

    // A subsequent valid full list still discovers the SDK 4/5 sink and yields stereo.
    n = build(sdk, NCMD);
    for (int i = 0; i < SAMPLES; i++) { mix[i] = 700; mix[SAMPLES + i] = -900; }
    assert(capture(list, n, out) == SAMPLES);
    for (int i = 0; i < SAMPLES; i++) assert(out[2 * i] == 700 && out[2 * i + 1] == -900);
    // Validation also applies after the histogram has already been logged.
    *(uint32_t *)(list + 4) = NCMD + 1; memset(list + n, 0, 0x40);
    assert(capture(list, n + 0x40, out) == 0);
}

static void test_histogram(void) {
    // 64 IDs x 1000 commands need 566 bytes of histogram text, more than its 512-byte buffer.
    // Disabled commands keep this a formatter test without dereferencing sink payloads.
    enum { COUNT = 64000, SIZE = 8 };
    size_t len = 0x20 + COUNT * SIZE;
    uint8_t *cl = calloc(1, len); assert(cl);
    *(uint32_t *)(cl + 4) = COUNT;
    for (int i = 0; i < COUNT; i++) { uint8_t *q = cl + 0x20 + i * SIZE; put(q, 16, i / 1000, SIZE); q[4] = 0; }
    FILE *log = tmpfile(); assert(log);
    fflush(stderr); int saved = dup(STDERR_FILENO); assert(saved >= 0);
    assert(dup2(fileno(log), STDERR_FILENO) >= 0);
    int16_t out[FRAME_MAX * 2]; assert(capture(cl, len, out) == 0);
    fflush(stderr); assert(dup2(saved, STDERR_FILENO) >= 0); close(saved);
    char text[2048]; assert(fseek(log, 0, SEEK_SET) == 0); size_t bytes = fread(text, 1, sizeof text - 1, log); text[bytes] = 0; fclose(log);
    const char *hist = strstr(text, "by command ID:"); assert(hist);
    hist += strlen("by command ID:");
    const char *end = strchr(hist, '\n'); assert(end && end - hist <= 512);
    assert(strstr(hist, " 0×1000 1×1000"));
    free(cl);
}

enum { TEST_SRC_SAMPLES = 22050, TEST_OUT_SAMPLES = 48000, TEST_ADPCM_BYTES = (TEST_SRC_SAMPLES / 14) * 8,
       TEST_SRC_32K = 32000, TEST_ADPCM_32K_BYTES = (TEST_SRC_32K / 14) * 8 + (TEST_SRC_32K % 14) / 2 + 1 + (TEST_SRC_32K % 14 & 1) };
static int32_t pcm_src[TEST_SRC_SAMPLES];
static int32_t pcm_32k[TEST_SRC_32K];
static uint8_t adpcm_src[TEST_ADPCM_BYTES];
static uint8_t adpcm_32k[TEST_ADPCM_32K_BYTES];
static int16_t pcm_whole[TEST_OUT_SAMPLES * 2], pcm_chunked[TEST_OUT_SAMPLES * 2];
static int16_t adpcm_whole[TEST_OUT_SAMPLES * 2], adpcm_chunked[TEST_OUT_SAMPLES * 2];
static int mix_stream(soft_voice *v, int16_t *out, const int *chunks, int nchunks) {
    int total = 0;
    for (int b = 0; b < nchunks; b++) {
        int n = chunks[b]; int32_t acc[FRAME_MAX * 2] = {0};
        soft_mix_voice(v, acc, n);
        for (int i = 0; i < n * 2; i++) out[2 * total + i] = (int16_t)acc[i];
        total += n;
    }
    return total;
}
static int make_chunks(int *chunks, int size) {
    int n = 0, left = TEST_OUT_SAMPLES;
    while (left) {
        int q = size;
        if (q > left) q = left;
        chunks[n++] = q; left -= q;
    }
    return n;
}
static void setup_voice(soft_voice *v, int fmt, int rate, const void *src, size_t bytes, uint32_t samples) {
    memset(v, 0, sizeof *v); v->used = 1; v->playing = 1; v->format = fmt; v->have_format = 1;
    v->channels = 1; v->rate = rate; v->volume = 1.f; v->pitch = 1.f; v->have_coef = 1;
    v->q[0].in.buffer = src; v->q[0].in.size = bytes; v->q[0].in.end = samples; v->qn = 1;
    v->q[0].user = src;
}
static void test_pcm_adpcm_stream(void) {
    for (int i = 0; i < TEST_SRC_SAMPLES; i++) pcm_src[i] = (i % 97 - 48) * 400 * 65536;
    memset(adpcm_src, 0x11, sizeof adpcm_src); adpcm_src[0] = 0; // valid frame headers and nonzero nibbles
    int baseline[TEST_OUT_SAMPLES / 240], chunks[TEST_OUT_SAMPLES];
    for (int i = 0; i < (int)(sizeof baseline / sizeof *baseline); i++) baseline[i] = 240;
    soft_voice v;
    setup_voice(&v, SF_PCM32, 22050, pcm_src, sizeof pcm_src, TEST_SRC_SAMPLES);
    assert(mix_stream(&v, pcm_whole, baseline, (int)(sizeof baseline / sizeof *baseline)) == TEST_OUT_SAMPLES);
    assert(v.played >= TEST_SRC_SAMPLES - 1 && v.played <= TEST_SRC_SAMPLES + 1);
    const int chunk_sizes[] = {1, 7, 240};
    for (size_t ci = 0; ci < sizeof chunk_sizes / sizeof *chunk_sizes; ci++) {
        int chunk = chunk_sizes[ci];
        int n = make_chunks(chunks, chunk);
        setup_voice(&v, SF_PCM32, 22050, pcm_src, sizeof pcm_src, TEST_SRC_SAMPLES);
        assert(mix_stream(&v, pcm_chunked, chunks, n) == TEST_OUT_SAMPLES);
        assert(!memcmp(pcm_whole, pcm_chunked, sizeof pcm_whole));
    }
    setup_voice(&v, SF_ADPCM, 22050, adpcm_src, sizeof adpcm_src, TEST_SRC_SAMPLES); memset(v.coef, 0, sizeof v.coef); v.coef[0] = v.coef[2] = 1024; v.coef[1] = v.coef[3] = -512;
    assert(mix_stream(&v, adpcm_whole, baseline, (int)(sizeof baseline / sizeof *baseline)) == TEST_OUT_SAMPLES);
    assert(v.played >= TEST_SRC_SAMPLES - 1 && v.played <= TEST_SRC_SAMPLES + 1);
    for (size_t ci = 0; ci < sizeof chunk_sizes / sizeof *chunk_sizes; ci++) {
        int chunk = chunk_sizes[ci];
        int n = make_chunks(chunks, chunk);
        setup_voice(&v, SF_ADPCM, 22050, adpcm_src, sizeof adpcm_src, TEST_SRC_SAMPLES); memset(v.coef, 0, sizeof v.coef); v.coef[0] = v.coef[2] = 1024; v.coef[1] = v.coef[3] = -512;
        assert(mix_stream(&v, adpcm_chunked, chunks, n) == TEST_OUT_SAMPLES);
        assert(!memcmp(adpcm_whole, adpcm_chunked, sizeof adpcm_whole));
    }
    // Decoder history and interpolation also survive a real WaveBuffer boundary.
    setup_voice(&v, SF_ADPCM, 22050, adpcm_src, 4000, 7000);
    v.coef[0] = v.coef[2] = 1024; v.coef[1] = v.coef[3] = -512;
    v.q[1].in.buffer = adpcm_src + 4000;
    v.q[1].in.size = sizeof adpcm_src - 4000;
    v.q[1].in.end = TEST_SRC_SAMPLES - 7000; v.qn = 2;
    assert(mix_stream(&v, adpcm_chunked, baseline, (int)(sizeof baseline / sizeof *baseline)) == TEST_OUT_SAMPLES);
    assert(!memcmp(adpcm_whole, adpcm_chunked, sizeof adpcm_whole));
    for (int i = 0; i < TEST_SRC_32K; i++) pcm_32k[i] = (i % 113 - 56) * 350 * 65536;
    setup_voice(&v, SF_PCM32, 32000, pcm_32k, sizeof pcm_32k, TEST_SRC_32K);
    assert(mix_stream(&v, pcm_whole, baseline, (int)(sizeof baseline / sizeof *baseline)) == TEST_OUT_SAMPLES);
    assert(v.played >= TEST_SRC_32K - 1 && v.played <= TEST_SRC_32K + 1);
    memset(adpcm_32k, 0x11, sizeof adpcm_32k); adpcm_32k[0] = 0;
    setup_voice(&v, SF_ADPCM, 32000, adpcm_32k, sizeof adpcm_32k, TEST_SRC_32K); memset(v.coef, 0, sizeof v.coef); v.coef[0] = v.coef[2] = 1024; v.coef[1] = v.coef[3] = -512;
    assert(mix_stream(&v, adpcm_whole, baseline, (int)(sizeof baseline / sizeof *baseline)) == TEST_OUT_SAMPLES);
    assert(v.played >= TEST_SRC_32K - 1 && v.played <= TEST_SRC_32K + 1);
}
static void test_soft_voice_hooks(void) {
    uint8_t voice = 0, config = 0; int16_t coef[16] = {0};
    acquire_result = 1;
    assert(h_ar_AcquireVoice(&config, &voice, 22050, 2, SF_PCM16, 7, coef, sizeof coef) == 1);
    assert(acquire_args[0] == 22050 && acquire_args[1] == 2 && acquire_args[2] == SF_PCM16 && acquire_args[3] == 7);
    soft_voice *v = soft_find(&voice, 0); assert(v && v->rate == 22050 && v->channels == 2 && v->format == SF_PCM16);
    acquire_result = 0; assert(h_ar_AcquireVoice(&config, &voice, 32000, 1, SF_PCM8, 3, NULL, 0) == 0);
    assert(v->rate == 22050); // failed acquisition must not install host state
    int16_t data[8] = {0}; uint8_t wb[0x30] = {0};
    *(const void **)wb = data; *(uint64_t *)(wb + 8) = sizeof data; *(uint32_t *)(wb + 0x14) = 4;
    append_calls = 0; assert(h_ar_AppendWB(&voice, wb) == 1 && append_calls == 0);
    assert(h_ar_WaveBufferCount(&voice) == 1);
    for (int i = 1; i < SOFT_WB; i++) assert(h_ar_AppendWB(&voice, wb) == 1);
    assert(h_ar_WaveBufferCount(&voice) == SOFT_WB);
    assert(h_ar_AppendWB(&voice, wb) == 0 && h_ar_WaveBufferCount(&voice) == SOFT_WB);
    soft_release_head(v);
    assert(h_ar_WaveBufferCount(&voice) == SOFT_WB - 1);
    assert(h_ar_AppendWB(&voice, wb) == 0); // A released buffer still occupies a slot until the guest collects it.
    assert(h_ar_GetReleasedWB(&voice) == wb);
    assert(h_ar_AppendWB(&voice, wb) == 1 && h_ar_WaveBufferCount(&voice) == SOFT_WB);
}
static void test_pcm16_single_sample_and_adpcm_edges(void) {
    int16_t pcm = -1234, out[2] = {0};
    soft_voice v; setup_voice(&v, SF_PCM16, 48000, &pcm, sizeof pcm, 1);
    assert(soft_looks_adpcm(sizeof pcm, 0, 1)); // Size alone resembles ADPCM; acquired PCM16 metadata must win.
    assert(soft_read_sample(&v, out) && out[0] == pcm && out[1] == pcm);

    const uint8_t encoded[] = {0x00, 0xf8, 0x70};
    int16_t coefs[16] = {0}, decoded[3] = {0}, h0 = 0, h1 = 0; uint8_t pred = 0;
    assert(soft_adpcm_decode(decoded, 3, encoded, sizeof encoded, 0, 3, 0, coefs, &pred, &h0, &h1) == 3);
    assert(decoded[0] == -1 && decoded[1] == -8 && decoded[2] == 7);

    const uint8_t invalid_pred[] = {0x80, 0x00};
    h0 = h1 = 0; pred = 0;
    assert(soft_adpcm_decode(decoded, 1, invalid_pred, sizeof invalid_pred, 0, 1, 0, coefs, &pred, &h0, &h1) == 0);
}

int main(int argc, char **argv) {
    test_pcm_adpcm_stream(); test_soft_voice_hooks(); test_pcm16_single_sample_and_adpcm_edges();
    if (argc == 2 && !strcmp(argv[1], "--histogram")) { test_histogram(); return 0; }
    if (argc == 2 && !strcmp(argv[1], "--malformed")) { test_malformed(16, 0); return 0; }
    test_headers();
    const int sdks[] = {4, 5, 6, 16, 21};
    for (size_t i = 0; i < sizeof sdks / sizeof *sdks; i++) for (int scenario = 0; scenario <= 4; scenario++) {
        // Each renderer format starts in a fresh process, including capture's one-time layout and sink detection.
        pid_t pid = fork(); assert(pid >= 0);
        if (!pid) {
            if (scenario <= 1) test(sdks[i], scenario);
            else test_malformed(sdks[i], scenario - 2);
            _exit(0);
        }
        int status; assert(waitpid(pid, &status, 0) == pid);
        assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
    }
    test_histogram();
    puts("audren: SDK command layouts, PCM/ADPCM streaming resampling across 1/7/240-sample blocks, one-second source duration and soft voice queue hooks passed");
    return 0;
}
