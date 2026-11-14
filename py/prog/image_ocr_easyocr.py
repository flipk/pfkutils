#!/usr/bin/env python3

import easyocr
import sys


def ocr_with_easyocr(image_path: str) -> str:
    """
    Performs OCR on a given PNG image using EasyOCR and returns
     the extracted text.

    Args:
        image_path: The file path to the PNG image.

    Returns:
        The OCR-scanned text as a string.
        Returns an error message if an exception occurs.
    """
    try:
        # Initialize the reader. You can specify languages,
        # e.g., ['en', 'ch_sim'] for English and Simplified Chinese
        # The first time you run it, it will download the necessary
        # language models.
        reader = easyocr.Reader(['en'])  # Specify English
        # detail=0 for just text, paragraph=True to join lines
        result = reader.readtext(image_path, detail=0, paragraph=True)
        return "\n".join(result)
    except FileNotFoundError:
        return f"Error: The file '{image_path}' was not found."
    except RuntimeError as e:
        # EasyOCR might raise RuntimeError for various issues
        # (e.g., model download failure)
        return (f"EasyOCR runtime error: {e}. Ensure you have an internet"
                f" connection for model downloads on first run.")
    except Exception as e:
        return f"An unexpected error occurred: {e}"


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: python {sys.argv[0]} <path_to_png_image>")
        sys.exit(1)

    image_file_path = sys.argv[1]

    if not image_file_path.lower().endswith(".png"):
        print("Error: The input file must be a PNG image.")
        sys.exit(1)

    scanned_text = ocr_with_easyocr(image_file_path)
    print("--- Scanned Text (EasyOCR) ---")
    print(scanned_text)
    print("------------------------------")
