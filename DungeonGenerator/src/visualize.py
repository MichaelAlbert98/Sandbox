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


def make_image(array, name):
    """
    turns a 3d array into an iamge and saves it with the given name

    Args:
        array: array, 3d array of dimensions [width][height][3]
        name: string, name to save the image as

    Returns:
        None
    """

    img = Image.fromarray(array)
    img = img.resize((img.width * TILESIZE, img.height * TILESIZE), resample=Image.BOX)
    img.save(name)


# example usage
if __name__ == "__main__":
    width = 50
    height = 50
    dungeon = DunGen.DungeonGenerator(width, height)
    dungeon.place_rooms_rand(3, 10, 200, 2, 1)
    print("rooms placed")
    dungeon.place_corridors(gen='l', curviness=0)
    print("corridors placed")
    for y in range(height):
        for x in range(width):
            if dungeon.valid_room(x, y, x, y, 1):
                dungeon.place_corridors(x, y, gen='l', curviness=0)
    print("extra corridors placed")
    connectors = dungeon.find_connectors()
    print("connectors found")
    dungeon.make_doorways(connectors, 0)
    print("doorways placed")
    make_image(map_to_pixel(np.array(dungeon.cells)).astype(np.uint8), "../images/prunepre.png")
    while dungeon.prune_deadends():
        pass
    print("deadends pruned")
    make_image(map_to_pixel(np.array(dungeon.cells)).astype(np.uint8), "../images/prunepost.png")
