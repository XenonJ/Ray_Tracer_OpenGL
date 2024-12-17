import numpy as np
import math

def rand2(grid_x, grid_y, seed=42):
    """Generate a pseudo-random value for a grid cell."""
    np.random.seed(hash((grid_x, grid_y, seed)) % (2**32))
    return np.random.rand(2)

def worley_noise(width, height, scale, layers=3, seed=42):
    """
    Generate Worley noise with multiple layers for fusion.

    Args:
        width (int): Width of the noise.
        height (int): Height of the noise.
        scale (int): Scale of the noise (distance between points).
        layers (int): Number of layers to blend for more randomness.
        seed (int): Seed for randomness.

    Returns:
        np.ndarray: 2D array of Worley noise values.
    """
    noise = np.zeros((height, width))
    for layer in range(layers):
        layer_seed = seed + layer  # Different seed for each layer
        for y in range(height):
            for x in range(width):
                cell_x = math.floor(x / scale)
                cell_y = math.floor(y / scale)

                min_dist = float('inf')
                for j in range(-1, 2):
                    for i in range(-1, 2):
                        # Neighboring cells
                        neighbor_cell_x = cell_x + i
                        neighbor_cell_y = cell_y + j

                        # Random point in the cell
                        random_point = rand2(neighbor_cell_x, neighbor_cell_y, seed=layer_seed)
                        random_x = neighbor_cell_x * scale + random_point[0] * scale
                        random_y = neighbor_cell_y * scale + random_point[1] * scale

                        # Distance to the point
                        dist = math.sqrt((x - random_x)**2 + (y - random_y)**2)
                        min_dist = min(min_dist, dist)

                noise[y, x] += min_dist

    # Normalize the noise to [0, 1]
    noise = (noise - noise.min()) / (noise.max() - noise.min())
    return noise

def generate_tiled_worley(width, height, tile_size, scale, layers, seed=42):
    """
    Generate tiled Worley noise by stitching smaller tiles with added randomness for each tile.

    Args:
        width (int): Total width of the tiled noise.
        height (int): Total height of the tiled noise.
        tile_size (int): Size of each individual tile (e.g., 128x128).
        scale (int): Scale of the noise (distance between points).
        layers (int): Number of layers to blend for more randomness.
        seed (int): Seed for randomness.

    Returns:
        np.ndarray: 2D array of tiled Worley noise.
    """
    num_tiles_x = width // tile_size
    num_tiles_y = height // tile_size
    tiled_noise = np.zeros((height, width))

    for tile_y in range(num_tiles_y):
        for tile_x in range(num_tiles_x):
            # Generate a single tile with unique seed for each tile
            tile_seed = seed + tile_y * num_tiles_x + tile_x
            noise_tile = worley_noise(tile_size, tile_size, scale, layers, seed=tile_seed)

            # Add randomness to each tile for blending
            randomness = np.random.normal(0, 0.1, (tile_size, tile_size))  # Small noise for blending
            noise_tile = np.clip(noise_tile + randomness, 0, 1)

            # Place the tile in the correct position
            start_x = tile_x * tile_size
            start_y = tile_y * tile_size
            tiled_noise[start_y:start_y + tile_size, start_x:start_x + tile_size] = noise_tile

    # Normalize the entire tiled noise to [0, 255]
    tiled_noise = (tiled_noise * 255).astype(np.uint8)
    return tiled_noise

def save_to_ppm(filename, noise):
    """
    Save a 2D noise array as a P3 PPM file.

    Args:
        filename (str): Output file name.
        noise (np.ndarray): 2D array of noise values.
    """
    height, width = noise.shape
    with open(filename, 'w') as f:
        f.write(f"P3\n{width} {height}\n255\n")
        for y in range(height):
            for x in range(width):
                value = noise[y, x]
                f.write(f"{value} {value} {value} ")
            f.write("\n")

# Parameters
width = 1024
height = 1024
tile_size = 128
scale = 32  # Controls the spacing of the Worley noise points
layers = 4  # Number of layers to blend for randomness
seed = 42  # Seed for reproducibility

# Generate tiled Worley noise
tiled_noise = generate_tiled_worley(width, height, tile_size, scale, layers, seed)

# Save to PPM
ppm_file_path = "tiled_worley_noise.ppm"
save_to_ppm(ppm_file_path, tiled_noise)

print(f"Worley noise saved to {ppm_file_path}")
