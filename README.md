# Label images

A tool for labelling images for object detection models

# How to use

Place your images (png, jpg, bmp) in the folder `dataset/` and run the `imageLabel.exe` program. The images will be loaded into the program (it may take a little bit of time for a large dataset)

You can then label the images with the mouse

To export the labels in the YOLO format, go to File -> Export. This will save a bunch of `.txt` files with names matching the images to the `labels/` folder.

# Ensure the following about your dataset

- All image are 640x640
- All images are RGB formatted (no transparency)