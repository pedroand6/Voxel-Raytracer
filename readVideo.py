import numpy as np

# Load data
b = np.load("bad_apple_480p.npz")
# Open in TEXT mode because we are writing characters
f = open("bad_apple.txt", "w") 

# Iterate frames
for frame in b['frames']:
    # Assuming 'frame' is shaped (480, 640) or (rows, cols)
    # Flatten it to write sequentially
    flat_frame = frame.flatten()
    
    for pixel in flat_frame:
        if pixel > 0: # Assuming your npz has numbers, not bools
            f.write('1')
        else:
            f.write('0')

f.close()