#!/bin/env python3

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

def plot_two_line_graphs(x_data,
                         y_data1, y_data2,
                         width_pixels, height_pixels,
                         filename_path,
                         title, y1_label, y2_label):
    if not (len(x_data) == len(y_data1) == len(y_data2)):
        raise ValueError("x_data, y_data1, and y_data2 must all have the same number of elements.")

    # Convert pixels to inches for Matplotlib's figsize argument.
    dpi = 100
    width_inches = width_pixels / dpi
    height_inches = height_pixels / dpi

    plt.figure(figsize=(width_inches, height_inches), dpi=dpi)

    # Plot the first line
    plt.plot(x_data, y_data1, label=y1_label)
    # Plot the second line
    plt.plot(x_data, y_data2, label=y2_label, linestyle='--')

    plt.xlabel("dBm@input")
    plt.ylabel("spur dBm")
    plt.title(title)
    plt.grid(True)
    plt.legend()
    plt.savefig(filename_path, bbox_inches='tight', pad_inches=0.1)
    plt.close()
    print(f"Plot {title} saved to {filename_path}")


def plot_three_line_graphs(x_data,
                           y_data1, y_data2, y_data3,
                           width_pixels, height_pixels,
                           filename_path,
                           title,
                           y12_axis_label, y3_axis_label,
                           y1_label, y2_label, y3_label,
                           y3_tick_spacing=None):
    """
    Plots three y-axis arrays on a single graph.
    y_data1 and y_data2 share the left y-axis.
    y_data3 uses a separate right y-axis.
    """
    if not (len(x_data) == len(y_data1) == len(y_data2) == len(y_data3)):
        raise ValueError("x_data and all y_data arrays must have the same number of elements.")

    # Convert pixels to inches for Matplotlib's figsize
    dpi = 100
    width_inches = width_pixels / dpi
    height_inches = height_pixels / dpi

    # Create a figure and the first axis (ax1)
    fig, ax1 = plt.subplots(figsize=(width_inches, height_inches), dpi=dpi)

    # Create a second axis that shares the same x-axis ---
    ax2 = ax1.twinx()

    # make the tics occur every 5 db on the right axis.
    if y3_tick_spacing:
        ax2.yaxis.set_major_locator(MultipleLocator(y3_tick_spacing))

    # Plot the first and second lines on the primary axes (ax1)
    # Assign specific colors to differentiate the lines and their corresponding axes
    line1 = ax1.plot(x_data, y_data1, label=y1_label, color='tab:blue')
    line2 = ax1.plot(x_data, y_data2, label=y2_label, linestyle='--', color='tab:orange')

    # Plot the third line on the secondary axes (ax2)
    line3 = ax2.plot(x_data, y_data3, label=y3_label, linestyle=':', color='tab:green')

    # Set labels for the primary (left) y-axis
    ax1.set_xlabel("dBm@input")
    ax1.set_ylabel(y12_axis_label, color='tab:blue')
    ax1.tick_params(axis='y', labelcolor='tab:blue')
    ax1.grid(True)

    # Set labels for the secondary (right) y-axis
    ax2.set_ylabel(y3_axis_label, color='tab:green')
    ax2.tick_params(axis='y', labelcolor='tab:green')

    # Combine legends from both axes ---
    # To create a single legend for all three lines, we gather the handles and labels
    # from both axes objects and pass them to the legend function.
    lines = line1 + line2 + line3
    labels = [l.get_label() for l in lines]
    ax1.legend(lines, labels, loc='upper left')

    plt.title(title)
    # Use tight_layout to ensure labels don't overlap
    fig.tight_layout()

    plt.savefig(filename_path, bbox_inches='tight', pad_inches=0.1)
    plt.close(fig)  # Close the figure object
    print(f"Plot '{title}' saved to {filename_path}")
