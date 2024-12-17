# https://www.cadhatch.com/seamless-water-textures

from PIL import Image

input_file = "input.jpg"
output_file = "output.ppm"

try:
    img = Image.open(input_file)
    img = img.convert("RGB") 

    target_size = (640, 640)
    img = img.resize(target_size)

    with open(output_file, "w") as f:
        f.write(f"P3\n{img.width} {img.height}\n255\n")
        for pixel in list(img.getdata()):
            f.write(f"{pixel[0]} {pixel[1]} {pixel[2]} ")
        f.write("\n")

    print(f"Converted {input_file} to P3 PPM format ({target_size}), saved as {output_file}")
except Exception as e:
    print(f"Error: {e}")
