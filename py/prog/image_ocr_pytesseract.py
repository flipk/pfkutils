#!/usr/bin/env python3
"""
image_ocr.py - Read an image file, perform OCR, and write extracted text to a TXT file.
"""

import argparse
import os
import sys
from pathlib import Path
from PIL import Image, ImageOps
import pytesseract


def preprocess_image(
    image: Image.Image,
    auto_invert: bool = True,
    scale_factor: float = 2.0,
    border_px: int = 20,
) -> Image.Image:
    """
    Preprocess image for optimal Tesseract OCR accuracy.
    - Converts to grayscale.
    - Automatically inverts if background is dark.
    - Scales up low-resolution images / screenshots.
    - Adds border padding to prevent edge text clipping.
    """
    # Convert to grayscale
    gray = image.convert("L")

    # Invert if the background is dark (average brightness < 128)
    if auto_invert:
        # Sample corners and overall mean
        w, h = gray.size
        corners = [
            gray.getpixel((0, 0)),
            gray.getpixel((w - 1, 0)),
            gray.getpixel((0, h - 1)),
            gray.getpixel((w - 1, h - 1)),
        ]
        avg_corner = sum(corners) / len(corners)
        if avg_corner < 128:
            gray = ImageOps.invert(gray)

    # Add border padding
    if border_px > 0:
        gray = ImageOps.expand(gray, border=border_px, fill=255)

    # Scale up if requested
    if scale_factor > 1.0:
        new_size = (int(gray.width * scale_factor), int(gray.height * scale_factor))
        gray = gray.resize(new_size, Image.Resampling.LANCZOS)

    return gray


def extract_text_from_image(
    image_path: str | Path,
    lang: str = "eng",
    psm: int = 3,
    preprocess: bool = True,
    scale_factor: float = 2.0,
) -> str:
    """
    Extract text from an image file using Tesseract OCR.
    """
    path = Path(image_path)
    if not path.is_file():
        raise FileNotFoundError(f"Image file not found: {image_path}")

    with Image.open(path) as img:
        if preprocess:
            processed = preprocess_image(img, scale_factor=scale_factor)
        else:
            processed = img

        config = f"--psm {psm} -c preserve_interword_spaces=1"
        text = pytesseract.image_to_string(processed, lang=lang, config=config)

    return text


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Extract text from an image using OCR and output to a TXT file."
    )
    parser.add_argument("input_image", help="Path to input image file (PNG, JPG, etc.)")
    parser.add_argument(
        "-o",
        "--output",
        default=None,
        help="Path to output text file (defaults to <image_stem>.txt)",
    )
    parser.add_argument(
        "-l",
        "--lang",
        default="eng",
        help="Tesseract language model (default: eng)",
    )
    parser.add_argument(
        "--psm",
        type=int,
        default=3,
        help="Page segmentation mode (default: 3 - fully automatic)",
    )
    parser.add_argument(
        "--scale",
        type=float,
        default=2.0,
        help="Scaling factor for preprocessing (default: 2.0)",
    )
    parser.add_argument(
        "--no-preprocess",
        action="store_true",
        help="Disable image preprocessing (grayscale/invert/rescale)",
    )
    parser.add_argument(
        "-p",
        "--print",
        dest="print_output",
        action="store_true",
        help="Print extracted text to terminal in addition to saving",
    )

    args = parser.parse_args()

    input_path = Path(args.input_image)
    if not input_path.exists():
        print(f"Error: file not found '{input_path}'", file=sys.stderr)
        return 1

    # Determine output file path
    if args.output:
        output_path = Path(args.output)
    else:
        output_path = input_path.with_suffix(".txt")

    print(f"Processing image: {input_path}")
    try:
        text = extract_text_from_image(
            input_path,
            lang=args.lang,
            psm=args.psm,
            preprocess=not args.no_preprocess,
            scale_factor=args.scale,
        )
    except Exception as e:
        print(f"Error during OCR processing: {e}", file=sys.stderr)
        return 1

    # Write output file
    output_path.write_text(text, encoding="utf-8")
    line_count = len(text.splitlines())
    char_count = len(text)
    print(f"Successfully extracted {char_count} characters ({line_count} lines).")
    print(f"Saved OCR output to: {output_path}")

    if args.print_output:
        print("\n" + "=" * 40 + " Extracted Text " + "=" * 40)
        print(text)
        print("=" * 96)

    return 0


if __name__ == "__main__":
    sys.exit(main())
