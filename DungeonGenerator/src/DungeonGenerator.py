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
        self.doors = []
        self.regions = {}
        self.region_num = 0

    def __iter__(self):
        for y in range(self.height):
            for x in range(self.width):
                yield x, y, self.cells[y][x]

    def can_carve(self, x, y, xi, yi):
        """
        takes a starting cell and a direction of movement and returns True if the
        surrounding tiles are all BLANK.

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
        returns the locations of the cells directly adjacent to the given coordinates.

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
        determines whether the directly adjacent cells are valid corridor cells.

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
        does not exceed the dungeon's height/width.

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
        rooms of random sizes between min and max. Populates self.rooms.

        Args:
            min_size: integer, minimum height/width
            max_size: integer, maximum height/width
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
                self.region_num += 1
                for y in range(room_height):
                    for x in range(room_width):
                        self.cells[start_y + y][start_x + x] = FLOOR
                        self.regions[(start_x + x, start_y + y)] = self.region_num
                self.rooms.append(Room(start_x, start_y, room_width, room_height))

    def place_corridors(self, x=None, y=None, gen='l'):
        """
        generates a maze through unoccupied tiles using a growing tree algorithm.

        Args:
            x: integer, starting x coord. None picks random EMPTY cell
            y: integer, starting y coord. None picks random EMPTY cell
            gen: string, which cell to pick when selecting how to grow the maze
                ('f' = first, 'l' = last, 'm' = middle, 'r' = random)

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
        self.regions[(x, y)] = 0
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
                self.regions[(xi, yi)] = 0
                self.corridors.append((xi, yi))
                cells.append((xi, yi))
            else:
                cells.remove((x, y))

    def find_connectors(self):
        """
        iterates through the dungeon finding any EMPTY cells which lie between two different regions and mapping the
        (x,y) coords to the regions they (potentially) will connect to.

        Returns:
            dictionary which maps (x,y) tuples to set<int> of region numbers
        """

        connectors = {}

        for y in range(self.height):
            for x in range(self.width):
                if self.cells[y][x] == BLANK:
                    connecting_regions = set()
                    neighbors = self.find_neighbors(x, y)
                    for n in neighbors:
                        region = self.regions.get(n)
                        if region is not None:
                            connecting_regions.add(region)
                    if len(connecting_regions) == 2:
                        connectors[(x, y)] = connecting_regions

        return connectors

    def make_doorways(self, connectors, chance=0):
        """
        picks a connector at random and makes the given coords a DOOR cell. The connected regions are merged into a
        single region using the dictionary merged_regions. Connectors that connect only to merged regions are found
        and pruned/made into doorways at random based on the value of chance. Populates self.doors.

        Args:
            connectors: dictionary with coord-adjacent region key-value pairs
            chance: odds of additional connectors being added when pruning. higher = less likely. 0 for no additions

        Returns:
            None
        """

        unmerged_regions = set()
        merged_regions = {}
        for i in range(self.region_num+1):
            unmerged_regions.add(i)
            merged_regions[i] = i

        # connect regions until all connected
        while len(unmerged_regions) > 1:
            coords, regions = rand.choice(list(connectors.items()))
            self.cells[coords[1]][coords[0]] = DOOR
            self.doors.append(coords)

            # merge regions
            connected_regions = [merged_regions[region] for region in regions]
            dest = connected_regions[0]
            sources = set(connected_regions[1:])

            for i in range(self.region_num):
                if merged_regions[i] in sources:
                    merged_regions[i] = dest

            # remove sources from unmerged_regions
            unmerged_regions.difference_update(sources)

            # find unneeded connectors
            delete_list = []
            for coords, regions in connectors.items():
                unique_regions = set()
                for region in regions:
                    unique_regions.add(merged_regions[region])
                if len(unique_regions) == 1:
                    delete_list.append(coords)

            # delete unneeded connectors
            for coord in delete_list:
                if chance != 0 and rand.randrange(chance) == 0:
                    self.cells[coord[1]][coord[0]] = DOOR
                    self.doors.append(coord)
                del connectors[coord]

        # # naive approach, leads to every region connecting to all adjacent regions.
        # while len(connectors):
        #     coords, regions = rand.choice(list(connectors.items()))
        #     self.cells[coords[1]][coords[0]] = DOOR
        #     connectors = {key: val for key, val in connectors.items() if val != regions}

