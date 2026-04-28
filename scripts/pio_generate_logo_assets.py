Import("env")

from io import BytesIO
from pathlib import Path
import sys

try:
    import cairosvg
    from PIL import Image
except ImportError as exc:
    raise SystemExit(
        "Missing logo generation dependencies. Install them in the PlatformIO Python environment with: "
        f"{sys.executable} -m pip install Pillow cairosvg"
    ) from exc


TARGET_SIZES = (32,)
ALPHA_THRESHOLD = 16
LUMA_THRESHOLD = 224
CANVAS_PADDING = 2
UPSCALE_FACTOR = 8


def cprint(*args, **kwargs):
    print("pio_generate_logo_assets.py:", *args, **kwargs)


PROJECT_ROOT = Path(env.subst("$PROJECT_DIR")).resolve()
SVG_DIR = PROJECT_ROOT / "assets" / "logos" / "svg"
XBM_DIR = PROJECT_ROOT / "assets" / "logos" / "xbm"


def render_svg_mask(svg_path: Path, size: int) -> Image.Image:
    render_size = size * UPSCALE_FACTOR
    png_bytes = cairosvg.svg2png(
        url=str(svg_path),
        output_width=render_size,
        output_height=render_size,
    )

    rgba = Image.open(BytesIO(png_bytes)).convert("RGBA")
    composed = Image.alpha_composite(Image.new("RGBA", rgba.size, (255, 255, 255, 255)), rgba)
    gray = composed.convert("L")
    alpha = rgba.getchannel("A")

    mask_values = []
    for alpha_value, gray_value in zip(alpha.getdata(), gray.getdata()):
        is_foreground = alpha_value > ALPHA_THRESHOLD and gray_value < LUMA_THRESHOLD
        mask_values.append(255 if is_foreground else 0)

    mask = Image.new("1", rgba.size, 0)
    mask.putdata(mask_values)
    bbox = mask.getbbox()
    if bbox is None:
        raise ValueError(f"No visible pixels found in {svg_path}")

    cropped = mask.crop(bbox)
    inner_size = max(1, size - (CANVAS_PADDING * 2))
    scale = min(inner_size / cropped.width, inner_size / cropped.height)
    resized = cropped.resize(
        (
            max(1, int(round(cropped.width * scale))),
            max(1, int(round(cropped.height * scale))),
        ),
        Image.Resampling.LANCZOS,
    ).convert("1")

    canvas = Image.new("1", (size, size), 0)
    offset_x = (size - resized.width) // 2
    offset_y = (size - resized.height) // 2
    canvas.paste(resized, (offset_x, offset_y))
    return canvas


def generate_xbm(svg_path: Path, size: int) -> Path:
    xbm_path = XBM_DIR / f"{svg_path.stem}_{size}.xbm"
    symbol_name = xbm_path.stem
    xbm_text = render_svg_mask(svg_path, size).tobitmap(symbol_name)
    if isinstance(xbm_text, bytes):
        xbm_text = xbm_text.decode("ascii")
    xbm_text = xbm_text.replace("static char ", "static unsigned char ")
    xbm_path.write_text(xbm_text, encoding="ascii")
    return xbm_path


def build_logo_assets(*_, **__):
    if not SVG_DIR.exists():
        cprint(f"SVG directory not found: {SVG_DIR}")
        return

    XBM_DIR.mkdir(parents=True, exist_ok=True)

    for svg_path in sorted(SVG_DIR.glob("*.svg")):
        for size in TARGET_SIZES:
            xbm_path = generate_xbm(svg_path, size)
            cprint(f"Generated {xbm_path.relative_to(PROJECT_ROOT)} from {svg_path.relative_to(PROJECT_ROOT)}")


build_logo_assets()