"""French versions of the new sequence wizard buttons.

    python make_fr.py <folder with the English PNGs> <output folder>

The English PNGs are the musical_seq, effect_seq, time_custom and quick_start
arrays of src-ui-wx/shared/utils/Images_png.h (and their _pressed variants),
written out as files. Needs Pillow and the Candara Bold font (Windows). The
output goes to src-ui-wx/shared/utils/Images_png_fr.h as byte arrays named
<name>_fr_png, read by LocalizedWizardButton() in BitmapCache.cpp.

The English text (white fill, black outline, dark shadow) is removed inside a
region, each removed pixel taking its row's background colour (the glossy
gradient is uniform across a row), then the French text is drawn in the same
style.
"""
import statistics
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

D = sys.argv[1] if len(sys.argv) > 1 else "."
OUT = sys.argv[2] if len(sys.argv) > 2 else D
FONT = "C:/Windows/Fonts/candarab.ttf"

# name: (regions to erase, where the text goes, lines, max font size, columns sampled for the background)
JOBS = {
    # The film strip starts at (108, 68) and the clapperboard right of x 131.
    "musical_seq": ([(4, 4, 131, 68), (131, 52, 135, 68), (4, 68, 108, 74)], (4, 4, 131, 70), ["Séquence", "musicale"], 27, (8, 108)),
    "effect_seq": ([(8, 2, 260, 31)], (8, 2, 260, 31), ["Séquence d'effets"], 26, None),
    "time_custom": ([(5, 3, 180, 46)], (5, 3, 180, 46), ["Personnalisé"], 27, None),
    "quick_start": ([(5, 3, 180, 46)], (5, 3, 180, 46), ["Démarrage rapide"], 27, None),
}


def text_mask(im, region):
    x0, y0, x1, y1 = region
    px = im.load()
    mask = Image.new("L", im.size, 0)
    mp = mask.load()
    for y in range(y0, y1):
        for x in range(x0, x1):
            r, g, b, a = px[x, y]
            if a < 200:
                continue
            bright = r > 200 and g > 200 and b > 200
            dark = max(r, g, b) < 90
            if bright or dark:
                mp[x, y] = 255
    # The anti-aliased edges and the soft shadow sit around the core pixels.
    mask = mask.filter(ImageFilter.MaxFilter(5))
    # Keep the mask inside the region.
    clip = Image.new("L", im.size, 0)
    ImageDraw.Draw(clip).rectangle((x0, y0, x1 - 1, y1 - 1), fill=255)
    return Image.composite(mask, Image.new("L", im.size, 0), clip)


def erase(im, mask, region, cols):
    x0, y0, x1, y1 = region
    px, mp = im.load(), mask.load()
    w = im.size[0]
    c0, c1 = cols if cols else (8, w - 8)
    # Background colour of each row, from the button's middle away from the
    # rounded border and the pictures. The gaps between letters count: only
    # greys (the letters' white fill, black outline and their anti-aliasing)
    # are left out, the background being a saturated colour.
    bg = {}
    for y in range(max(0, y0 - 6), min(im.size[1], y1 + 6)):
        samples = [px[x, y] for x in range(c0, c1)
                   if px[x, y][3] == 255 and 90 <= max(px[x, y][:3]) and max(px[x, y][:3]) - min(px[x, y][:3]) > 60]
        if len(samples) >= 5:
            # The letters' shadow darkens part of the row: keep its brighter half.
            samples.sort(key=lambda s: sum(s[:3]), reverse=True)
            samples = samples[: max(5, len(samples) // 2)]
            bg[y] = tuple(statistics.median(s[c] for s in samples) for c in range(3))
    known = sorted(bg)
    for y in range(y0, y1):
        if y in bg:
            colour = bg[y]
        else:
            # Rows covered by letters: the gloss is a vertical gradient, so
            # interpolate between the nearest rows that show the background.
            above = [k for k in known if k < y]
            below = [k for k in known if k > y]
            if above and below:
                a, b = above[-1], below[0]
                t = (y - a) / (b - a)
                colour = tuple(bg[a][c] + (bg[b][c] - bg[a][c]) * t for c in range(3))
            elif above or below:
                colour = bg[(above or below)[-1 if above else 0]]
            else:
                continue
        fill = tuple(int(round(c)) for c in colour) + (255,)
        for x in range(x0, x1):
            if mp[x, y]:
                px[x, y] = fill


def draw_text(im, region, lines, max_size):
    x0, y0, x1, y1 = region
    d = ImageDraw.Draw(im)
    size = max_size
    while size > 10:
        font = ImageFont.truetype(FONT, size)
        widths = [d.textlength(t, font=font) for t in lines]
        asc, desc = font.getmetrics()
        line_h = asc + desc - 4
        if max(widths) <= (x1 - x0) - 8 and line_h * len(lines) <= (y1 - y0):
            break
        size -= 1
    total = line_h * len(lines)
    y = y0 + ((y1 - y0) - total) / 2 - 2
    for t, tw in zip(lines, widths):
        x = x0 + ((x1 - x0) - tw) / 2
        d.text((x + 1, y + 2), t, font=font, fill=(0, 0, 0, 150), stroke_width=2, stroke_fill=(0, 0, 0, 150))
        d.text((x, y), t, font=font, fill="white", stroke_width=2, stroke_fill="black")
        y += line_h
    return size


for name, (regions, text_region, lines, max_size, cols) in JOBS.items():
    for suffix in ("", "_pressed"):
        im = Image.open(f"{D}/{name}{suffix}.png").convert("RGBA")
        for region in regions:
            mask = text_mask(im, region)
            erase(im, mask, region, cols)
        size = draw_text(im, text_region, lines, max_size)
        im.save(f"{OUT}/{name}{suffix}_fr.png", optimize=True)
        print(f"{name}{suffix}_fr.png  font {size}px")
