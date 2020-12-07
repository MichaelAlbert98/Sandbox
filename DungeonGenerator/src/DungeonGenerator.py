import random as rand

# cell constants
BLANK = 0
FLOOR = 1
CORRIDOR = 2
PATHEND = 3
WALL = 4
DOOR = 5
CAVE = 6


class Room:
    """
    container to hold dungeon rooms for later access without searching the
    entire grid space.

    Args:
        x, y: integer, lower left coords for the room
        height, width: intger, height/width for the room

    Attrs:
        x,y: integer, coords in 2d array
        width, height: integer, horizontal/vertical cell span
    """

    def __init__(self, x, y, width, height):
        self.x = x
        self.y = y
        self.width = width
        self.height = height


class DungeonGenerator:
    """

    """

    def __init__(self, width, height):
        self.width = width
        self.height = height
        self.cells = [[BLANK for _ in range(width)] for _ in range(height)]
        self.rooms = []
        self.corridors = []

    def __iter__(self):
        for y in range(self.height):
            for x in range(self.width):
                yield x, y, self.cells[y][x]

    def can_carve(self, x, y, xi, yi):
        """
        takes a starting cell and a direction of movement and returns True if the
        surrounding tiles are all BLANK

        Args:
            x: integer, starting x coord
            y: integer, starting y coord
            xi: integer, movement direction (-1, 0, or 1)
            yi: integer, movement direction (-1, 0, or 1)

        Returns:
            True if the tiles around the direction of movement are BLANK
        """

        xj = (-1, 0, 1) if xi == 0 else (xi, 2 * xi)
        yj = (-1, 0, 1) if yi == 0 else (yi, 2 * yi)
        for b in yj:
            for a in xj:
                if self.cells[y + b][x + a] != BLANK:
                    return False
        return True

    def find_neighbors(self, x, y):
        """
        returns the locations of the cells directly adjacent to the given coordinates

        Args:
            x: integer, starting x coord
            y: integer, starting y coord

        Returns:
            list of (x,y) tuples indicating adjacent cells
        """

        neighbors = []
        cells = (-1, 0, 1)
        for i in cells:
            for j in cells:
                if abs(j) != abs(i):
                    neighbors.append((x + i, y + j))
        return neighbors

    def find_moves(self, x, y):
        """
        determines whether the directly adjacent cells are valid
        corridor cells

        Args:
            x: integer, starting x coord
            y: integer, starting y coord

        Returns:
            list of tuples that are valid corridor cells
        """

        pos_moves = []
        neighbors = self.find_neighbors(x, y)
        for xi, yi in neighbors:
            if xi < 1 or yi < 1 or xi > self.width - 2 or yi > self.height - 2:
                continue
            if self.can_carve(x, y, xi - x, yi - y):
                pos_moves.append((xi, yi))
        return pos_moves

    # dungeon generator methods
    def valid_room(self, xi, yi, xj, yj, valid_dist):
        """
        return True if the given rectangle overlaps entirely with EMPTY tiles and
        does not exceed the dungeon's height/width

        Args:
            xi, yi: integer, lower left rectangle position
            xj, yj: integer, upper right rectangle position
            valid_dist: distance rooms must be from non-EMPTY cells to be valid

        Returns:
            True if only overlapping EMPTY cells
        """

        xi -= valid_dist
        yi -= valid_dist
        xj += valid_dist
        yj += valid_dist
        if xi >= 0 and yi >= 0 and xj < self.width and yj < self.height:
            for x in range(xj - xi):
                for y in range(yj - yi):
                    if self.cells[yi + y][xi + x]:
                        return False
            return True
        return False

    def place_rooms_rand(self, min_size, max_size, attempts, step=1, valid_dist=1):
        """
        randomly places rooms with a brute force method, trying attempt times with
        rooms of random sizes between min and max. Fills self.rooms

        Args:
            min_size, max_size: integer, minimum/maximum height/width
            attempts: integer, number of placements tried
            step: integer, how the room size scales
            valid_dist: integer, distance rooms must be from non-EMPTY cells to be valid

        Returns:
            None
        """

        for _ in range(attempts):
            start_x = rand.randrange(self.width)
            start_y = rand.randrange(self.height)
            room_width = rand.randrange(min_size, max_size, step)
            room_height = rand.randrange(min_size, max_size, step)
            if self.valid_room(start_x, start_y, start_x + room_width, start_y + room_height, valid_dist):
                for y in range(room_height):
                    for x in range(room_width):
                        self.cells[start_y + y][start_x + x] = FLOOR
                self.rooms.append(Room(start_x, start_y, room_width, room_height))

    def place_corridors(self, x=None, y=None, gen='l'):
        """
        generates a maze through unoccupied tiles using a growing tree algorithm

        Args:
            x: integer, starting x coord. None picks random EMPTY cell
            y: integer, starting y coord. None picks random EMPTY cell
            gen: string, which cell to pick when selecting how to grow the maze
                ('f' = first, 'l' = last, 'm' = middle, 'r' = random,)

        Returns:
            None
        """

        cells = []
        if x is None and y is None:
            x = rand.randrange(1, self.width - 2)
            y = rand.randrange(1, self.height - 2)
            while not self.can_carve(x, y, 0, 0):
                x = rand.randrange(self.width)
                y = rand.randrange(self.height)

        self.cells[y][x] = CORRIDOR
        cells.append((x, y))

        while cells:
            if gen == 'l':
                x, y = cells[-1]  # last cell
            elif gen == 'f':
                x, y = cells[0]  # first cell
            elif gen == 'm':
                x, y = cells[len(cells) // 2]  # middle cell
            else:
                x, y = cells[rand.randrange(len(cells))]  # random cell

            pos_moves = self.find_moves(x, y)
            if pos_moves:
                xi, yi = rand.choice(pos_moves)
                self.cells[yi][xi] = CORRIDOR
                self.corridors.append((xi, yi))
                cells.append((xi, yi))
            else:
                cells.remove((x, y))
