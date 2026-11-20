// ui-rects: find rectangular UI elements (buttons, fields, panels, cards) in a screenshot so the
// XerahS slurp wrapper can snap to them like ShareX does on Windows. Wayland gives no access to
// other apps' widget trees, so this works on pixels: mark colour edges, flood-fill the regions
// between them and keep regions that fill their bounding box once holes are ignored, i.e. a
// shape with straight borders. Text inside a button only punches holes, the outline still counts.
//
// Usage: grim -s 1 -t ppm - | ui-rects [offset_x offset_y]
// Prints "x,y wxh" per element (slurp's box format), in layout coordinates.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDGE_DIFF 7       // max channel difference between pixels that still counts as flat
#define MIN_W 18
#define MIN_H 12
#define RECT_FILL 0.90    // share of the bbox covered by the row spans of the region
#define MAX_RECTS 4000
#define JOIN_GAP 4        // cells this close (a grid line between them) belong to one element
#define ALIGN 3           // px of slack when comparing cell edges
#define JOIN_FILL 0.85    // a joined group must cover this much of its outline

static int read_int(FILE *f) {
    int c, v = 0;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(f)) != EOF && c != '\n') {}
            continue;
        }
        if (c >= '0' && c <= '9') break;
    }
    if (c == EOF) return -1;
    do v = v * 10 + (c - '0'); while ((c = fgetc(f)) >= '0' && c <= '9');
    return v;
}

static inline int differs(uint32_t a, uint32_t b) {
    if (a == b) return 0; // flat UI: by far the common case
    for (int sh = 0; sh < 24; sh += 8)
        if (abs((int)(a >> sh & 255) - (int)(b >> sh & 255)) > EDGE_DIFF) return 1;
    return 0;
}

typedef struct { int x0, y0, x1, y1; long n; } Region;

int main(int argc, char **argv) {
    int ox = argc > 2 ? atoi(argv[1]) : 0, oy = argc > 2 ? atoi(argv[2]) : 0;
    char magic[3] = {0};
    if (fread(magic, 1, 2, stdin) != 2 || strcmp(magic, "P6")) return 1;
    int w = read_int(stdin), h = read_int(stdin), maxv = read_int(stdin);
    if (w <= 0 || h <= 0 || maxv != 255) return 1;
    size_t n = (size_t)w * h;
    uint8_t *px = malloc(n * 3);
    if (!px || fread(px, 3, n, stdin) != n) return 1;

    // Edge map: a pixel is an edge if it differs from the pixel 1 or 2 to the right/below.
    // Comparing over 2 px too catches anti-aliased borders (0 -> 13 -> 18) whose single steps
    // stay under the threshold and would let neighbouring panels leak into each other.
    uint32_t *rgb = malloc(n * sizeof *rgb);
    for (size_t i = 0; i < n; i++) rgb[i] = px[i * 3] | px[i * 3 + 1] << 8 | (uint32_t)px[i * 3 + 2] << 16;
    free(px);
    uint8_t *edge = calloc(n, 1);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            const uint32_t *p = rgb + (size_t)y * w + x;
            if ((x + 1 < w && differs(p[0], p[1])) || (x + 2 < w && differs(p[0], p[2])) ||
                (y + 1 < h && differs(p[0], p[w])) || (y + 2 < h && differs(p[0], p[2 * (size_t)w])))
                edge[(size_t)y * w + x] = 1;
        }

    // Label flat regions (4-connected) with an explicit stack.
    int32_t *label = malloc(n * sizeof *label);
    for (size_t i = 0; i < n; i++) label[i] = -1;
    uint32_t *stack = malloc(n * sizeof *stack);
    size_t cap = 1024, count = 0;
    Region *reg = malloc(cap * sizeof *reg);
    for (size_t s = 0; s < n; s++) {
        if (edge[s] || label[s] >= 0) continue;
        if (count == cap) reg = realloc(reg, (cap *= 2) * sizeof *reg);
        Region r = { (int)(s % w), (int)(s / w), (int)(s % w), (int)(s / w), 0 };
        size_t sp = 0;
        stack[sp++] = (uint32_t)s;
        label[s] = (int32_t)count;
        while (sp) {
            uint32_t i = stack[--sp];
            int x = i % w, y = i / w;
            r.n++;
            if (x < r.x0) r.x0 = x;
            if (x > r.x1) r.x1 = x;
            if (y < r.y0) r.y0 = y;
            if (y > r.y1) r.y1 = y;
            uint32_t nb[4]; int k = 0;
            if (x > 0) nb[k++] = i - 1;
            if (x + 1 < w) nb[k++] = i + 1;
            if (y > 0) nb[k++] = i - w;
            if (y + 1 < h) nb[k++] = i + w;
            while (k--) if (!edge[nb[k]] && label[nb[k]] < 0) { label[nb[k]] = (int32_t)count; stack[sp++] = nb[k]; }
        }
        reg[count++] = r;
    }

    // Rectangularity: for every row of a region take the span from its first to its last pixel.
    // Holes inside (text, icons, list images) do not matter, rounded corners cost little, but
    // L shapes, diagonals and blobs fall short of the bounding box. (A column pass would catch a
    // few more odd shapes but costs ~80 ms in cache misses.)
    typedef struct { int cur, lo, hi; long sum; } Span;
    Span *rs = calloc(count, sizeof *rs);
    for (size_t c = 0; c < count; c++) rs[c].cur = -1;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int32_t c = label[(size_t)y * w + x];
            if (c < 0) continue;
            if (rs[c].cur != y) {
                if (rs[c].cur >= 0) rs[c].sum += rs[c].hi - rs[c].lo + 1;
                rs[c].cur = y;
                rs[c].lo = x;
            }
            rs[c].hi = x;
        }

    typedef struct { int x0, y0, x1, y1; } Box; // inclusive
    Box *out = malloc(MAX_RECTS * sizeof *out);
    int nout = 0;
    for (size_t c = 0; c < count && nout < MAX_RECTS; c++) {
        Region r = reg[c];
        int rw = r.x1 - r.x0 + 1, rh = r.y1 - r.y0 + 1;
        if (rw < MIN_W || rh < MIN_H || (rw >= w - 2 && rh >= h - 2)) continue;
        double area = (double)rw * rh;
        double rows = (rs[c].sum + rs[c].hi - rs[c].lo + 1) / area;
        if (rows < RECT_FILL) continue;
        // Grow by the 1 px edge line around the flat area, so the element's border is included.
        out[nout++] = (Box){ r.x0 > 0 ? r.x0 - 1 : 0, r.y0 > 0 ? r.y0 - 1 : 0,
                             r.x1 + 1 < w ? r.x1 + 1 : r.x1, r.y1 + 1 < h ? r.y1 + 1 : r.y1 };
    }

    // Cells split only by thin lines (tables, grids, segmented bars) form one element: join
    // boxes that share an edge across a gap of at most JOIN_GAP px and report the group's
    // outline instead of its cells, if the group fills its bounding box.
    int *parent = malloc(nout * sizeof *parent);
    for (int i = 0; i < nout; i++) parent[i] = i;
    for (int i = 0; i < nout; i++)
        for (int j = i + 1; j < nout; j++) {
            Box a = out[i], b = out[j];
            int gx = (a.x0 > b.x1 ? a.x0 - b.x1 : b.x0 - a.x1) - 1; // free columns between
            int gy = (a.y0 > b.y1 ? a.y0 - b.y1 : b.y0 - a.y1) - 1;
            // Grid cells share a whole edge: same row (top and bottom) side by side, or the
            // same column (left and right) stacked.
            int row = abs(a.y0 - b.y0) <= ALIGN && abs(a.y1 - b.y1) <= ALIGN;
            int col = abs(a.x0 - b.x0) <= ALIGN && abs(a.x1 - b.x1) <= ALIGN;
            int side = row && gx >= -1 && gx <= JOIN_GAP;
            int stack_ = col && gy >= -1 && gy <= JOIN_GAP;
            if (!side && !stack_) continue;
            int ri = i, rj = j;
            while (parent[ri] != ri) ri = parent[ri];
            while (parent[rj] != rj) rj = parent[rj];
            if (ri != rj) parent[rj] = ri;
        }
    for (int i = 0; i < nout; i++) {
        int root = i;
        while (parent[root] != root) root = parent[root];
        parent[i] = root;
    }
    for (int g = 0; g < nout; g++) {
        Box bb = { w, h, -1, -1 };
        double cells = 0;
        int members = 0;
        for (int i = 0; i < nout; i++) {
            if (parent[i] != g) continue;
            Box a = out[i];
            members++;
            cells += (double)(a.x1 - a.x0 + 1) * (a.y1 - a.y0 + 1);
            if (a.x0 < bb.x0) bb.x0 = a.x0;
            if (a.y0 < bb.y0) bb.y0 = a.y0;
            if (a.x1 > bb.x1) bb.x1 = a.x1;
            if (a.y1 > bb.y1) bb.y1 = a.y1;
        }
        if (!members) continue;
        double bbox = (double)(bb.x1 - bb.x0 + 1) * (bb.y1 - bb.y0 + 1);
        if (members > 1 && cells >= JOIN_FILL * bbox) {
            printf("%d,%d %dx%d\n", bb.x0 + ox, bb.y0 + oy, bb.x1 - bb.x0 + 1, bb.y1 - bb.y0 + 1);
            continue;
        }
        for (int i = 0; i < nout; i++)
            if (parent[i] == g)
                printf("%d,%d %dx%d\n", out[i].x0 + ox, out[i].y0 + oy,
                       out[i].x1 - out[i].x0 + 1, out[i].y1 - out[i].y0 + 1);
    }
    return 0;
}
