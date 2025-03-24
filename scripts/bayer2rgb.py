# ----------------------------------------------------------------------
#
# File: bayer2rgb.py
#
# Last edited: 16.11.2024
#
# Copyright (c) 2024 ETH Zurich and University of Bologna
#
# Authors:
# - Philip Wiese (wiesep@iis.ee.ethz.ch), ETH Zurich
#
# ----------------------------------------------------------------------
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the License); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an AS IS BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse

import cv2
import numpy as np


def main():
    # Set up argument parsing
    parser = argparse.ArgumentParser(description = "Process and display the shape of an image from a binary file.")
    parser.add_argument('--input_file', type = str, default = 'Out.bin', help = 'Path to the input binary file')
    parser.add_argument('--output_file', type = str, default = None, help = 'Path to save the output image')
    args = parser.parse_args()

    # Extract arguments
    input_file = args.input_file
    output_file = input_file.split(".")[0] + ".png" if args.output_file is None else args.output_file  # Default

    # Read and process the binary file
    try:
        with open(input_file, "rb") as rawimg:
            # Load PGM header
            # image = cv2.imread(input_file, cv2.IMREAD_UNCHANGED)
            image = np.fromfile(input_file, np.uint8, 256 * 256 * 3)

            image = image.reshape((256, 256, 3))
            # Print the shape of the image
            print(f"Shape of the image: {image.shape}")

            image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)

            # Check if the image is grayscale
            if len(image.shape) == 2:
                # Debayer image
                image = cv2.cvtColor(image, cv2.COLOR_BayerBGGR2BGR
                                    )  # The result is BGR format with 16 bits per pixel and 12 bits range [0, 2^10-1].
            else:
                print("The image is not grayscale.")

            # Save the RGB image
            cv2.imwrite(output_file, image)
            print(f"Image saved to {output_file}")

    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found.")
    except Exception as e:
        print(f"An error occurred: {e}")


if __name__ == "__main__":
    main()
