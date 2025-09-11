#!/usr/bin/env python3

# "vibe-coded" by the programming duo of Andy and Gemini Pro, 9/6/25
# Python 3.12 was used, after an unsuccessful attempt using Python 3.10
# Later "vibe-compiled" into a Windows executable using pyinstaller

# PFK: ported to use "pymupdf" instead of old "fitz"
# and cleaned up all pycharm warnings

# requirements:
# pillow
# pymupdf

import tkinter as tk
from tkinter import filedialog, messagebox
import pymupdf
from PIL import Image, ImageChops
import os
import threading


class PdfComparatorApp:
    def __init__(self, master):
        self.master = master
        master.title("PDF Comparison Tool")
        master.geometry("500x250")

        self.file1_path = tk.StringVar(value="No file selected")
        self.file2_path = tk.StringVar(value="No file selected")
        self.status_text = tk.StringVar(value="Ready")
        self._full_path1 = ""
        self._full_path2 = ""

        # --- GUI Widgets ---
        tk.Label(master, text="File 1:",
                 font=('Helvetica', 10, 'bold')).grid(row=0, column=0, padx=10, pady=10, sticky='w')
        tk.Label(master, textvariable=self.file1_path, fg="blue").grid(row=0, column=1, padx=10, pady=10, sticky='w')
        tk.Button(master, text="Select File 1...", command=self.select_file1).grid(row=0, column=2, padx=10, pady=10)

        tk.Label(master, text="File 2:",
                 font=('Helvetica', 10, 'bold')).grid(row=1, column=0, padx=10, pady=10, sticky='w')
        tk.Label(master, textvariable=self.file2_path, fg="blue").grid(row=1, column=1, padx=10, pady=10, sticky='w')
        tk.Button(master, text="Select File 2...", command=self.select_file2).grid(row=1, column=2, padx=10, pady=10)

        self.compare_button = tk.Button(master, text="Compare PDFs", font=('Helvetica', 12, 'bold'),
                                        command=self.start_comparison_thread)
        self.compare_button.grid(row=2, column=0, columnspan=3, pady=20)

        tk.Label(master, text="Status:",
                 font=('Helvetica', 10, 'bold')).grid(row=3, column=0, padx=10, pady=10, sticky='w')
        tk.Label(master, textvariable=self.status_text, font=('Helvetica', 10, 'italic')).grid(
            row=3, column=1, columnspan=2, padx=10, pady=10, sticky='w')

    def select_file1(self):
        path = filedialog.askopenfilename(filetypes=[("PDF Files", "*.pdf")])
        if path:
            self.file1_path.set(os.path.basename(path))
            self._full_path1 = path

    def select_file2(self):
        path = filedialog.askopenfilename(filetypes=[("PDF Files", "*.pdf")])
        if path:
            self.file2_path.set(os.path.basename(path))
            self._full_path2 = path

    def start_comparison_thread(self):
        # Run the comparison in a separate thread to keep the GUI responsive
        thread = threading.Thread(target=self.run_comparison)
        thread.daemon = True
        thread.start()

    def run_comparison(self):
        if not hasattr(self, '_full_path1') or not hasattr(self, '_full_path2'):
            messagebox.showerror("Error", "Please select two PDF files to compare.")
            return

        self.status_text.set("Processing... Please wait.")
        self.compare_button.config(state="disabled")

        try:
            # Step 1: Render PDFs to a list of PIL Images
            self.status_text.set("Rendering File 1...")
            doc1_images = self._render_pdf_to_images(self._full_path1)
            
            self.status_text.set("Rendering File 2...")
            doc2_images = self._render_pdf_to_images(self._full_path2)

            if len(doc1_images) != len(doc2_images):
                messagebox.showwarning("Warning",
                                       "Files have a different number of pages "
                                       f"({len(doc1_images)} vs {len(doc2_images)}). "
                                       "Comparing up to the shorter document.")
            
            # Step 2: Compare images and create difference images
            diff_images = []
            num_pages_to_compare = min(len(doc1_images), len(doc2_images))

            for i in range(num_pages_to_compare):
                self.status_text.set(f"Comparing page {i+1}/{num_pages_to_compare}...")
                img1 = doc1_images[i]
                img2 = doc2_images[i]

                # Create the "red on white" difference image
                difference = ImageChops.difference(img1, img2)
                diff_img = Image.new('RGB', img1.size, 'white')
                
                # This can be slow, but it's accurate
                diff_pixels = difference.load()
                output_pixels = diff_img.load()
                
                for x in range(img1.width):
                    for y in range(img1.height):
                        if diff_pixels[x, y] != (0, 0, 0):
                            output_pixels[x, y] = (255, 0, 0)  # Red
                
                diff_images.append(diff_img)

            # Step 3: Save the final PDF
            if not diff_images:
                messagebox.showinfo("No Differences", "No pages were compared or the files were identical.")
                return

            base1 = os.path.splitext(os.path.basename(self._full_path1))[0]
            base2 = os.path.splitext(os.path.basename(self._full_path2))[0]
            output_filename = f"{base1}_vs_{base2}.pdf"
            
            self.status_text.set(f"Saving final PDF as {output_filename}...")
            diff_images[0].save(
                output_filename,
                save_all=True,
                append_images=diff_images[1:]
            )

            messagebox.showinfo("Success", f"Comparison complete!\nOutput saved as:\n{output_filename}")

        except Exception as e:
            messagebox.showerror("An Error Occurred", str(e))
        finally:
            self.status_text.set("Ready")
            self.compare_button.config(state="normal")

    @staticmethod
    def _render_pdf_to_images(pdf_path):
        doc = pymupdf.open(pdf_path)
        images = []
        for page in doc:
            pix = page.get_pixmap(dpi=300)
            img = Image.frombytes("RGB", (pix.width, pix.height), pix.samples)
            images.append(img)
        doc.close()
        return images


if __name__ == "__main__":
    root = tk.Tk()
    app = PdfComparatorApp(root)
    root.mainloop()
