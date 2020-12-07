import DungeonGenerator as DunGen
import numpy as np
from PIL import Image

# constants
TILESIZE = 16


def map_to_pixel(dun_cells):
    """
    function which maps integer constants from DungeonGenerator to 3d pixel values

    Args:
        dun_cells: 2d array containing DunGen constants (0-6)

    Returns:
        3d array containing pixel values based on the DunGen constant
    """

    conversion_dict = {DunGen.BLANK: np.array([0, 0, 0]), DunGen.FLOOR: np.array([0, 150, 0]),
                       DunGen.CORRIDOR: np.array([50, 150, 255]),
                       DunGen.PATHEND: np.array([255, 0, 0]), DunGen.WALL: np.array([255, 255, 0]),
                       DunGen.DOOR: np.array([175, 80, 200]), DunGen.CAVE: np.array([150, 75, 20])}
    pixels = np.zeros((dun_cells.shape[0], dun_cells.shape[1], 3))
    for ix, iy in np.ndindex(dun_cells.shape):
        pixels[ix, iy] = conversion_dict.get(dun_cells[ix, iy])
    return pixels


# example usage
if __name__ == "__main__":
    dungeon = DunGen.DungeonGenerator(50, 50)
    dungeon.place_rooms_rand(3, 15, 100, 2, 1)
    dungeon.place_corridors(type='f')
    dun_image = map_to_pixel(np.array(dungeon.cells)).astype(np.uint8)
    img = Image.fromarray(dun_image)
    img = img.resize((img.width * TILESIZE, img.height * TILESIZE), resample=Image.BOX)
    img.save("../images/test.png")
