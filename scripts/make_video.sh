#!/bin/bash

ffmpeg \
 -framerate 15 \
 -pattern_type glob \
 -i 'fb/*.png' \
 -c:v libx264 \
 -pix_fmt yuv420p \
 -vf "scale=iw*4:ih*4:flags=neighbor" \
 -y \
 output.mp4