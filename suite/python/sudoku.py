"""The `sudoku` kernel of the puzzle group (kernels/SPEC.md section 13): Knuth's Algorithm X with dancing links."""

from gen import XorShift32, h31


def shuffle(rng: XorShift32, items: list) -> None:
    for i in range(len(items) - 1, 0, -1):
        j = rng.next() % (i + 1)
        items[i], items[j] = items[j], items[i]


SUDOKU_CLUES = 26
DLX_COLUMNS = 324


def sudoku_puzzles(n: int) -> list[list[int]]:
    rng = XorShift32()
    puzzles = []
    for _ in range(n):
        digits = list(range(1, 10))
        shuffle(rng, digits)
        order = []
        for _axis in range(2):
            bands = [0, 1, 2]
            shuffle(rng, bands)
            lines = []
            for band in bands:
                inner = [0, 1, 2]
                shuffle(rng, inner)
                lines += [band * 3 + i for i in inner]
            order.append(lines)
        rows, cols = order
        cells = list(range(81))
        shuffle(rng, cells)
        puzzle = [0] * 81
        for cell in cells[:SUDOKU_CLUES]:
            r, c = rows[cell // 9], cols[cell % 9]
            puzzle[cell] = digits[(r * 3 + r // 3 + c) % 9]
        puzzles.append(puzzle)
    return puzzles


class Dlx:
    """The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row."""

    def __init__(self) -> None:
        size = 1 + DLX_COLUMNS + 729 * 4
        self.left = [0] * size
        self.right = [0] * size
        self.up = list(range(size))
        self.down = list(range(size))
        self.col = list(range(size))
        self.row = [0] * size
        self.count = [0] * (DLX_COLUMNS + 1)
        for i in range(DLX_COLUMNS + 1):
            self.left[i] = i - 1
            self.right[i] = i + 1
        self.left[0] = DLX_COLUMNS
        self.right[DLX_COLUMNS] = 0
        node = DLX_COLUMNS + 1
        for r in range(9):
            for c in range(9):
                box = (r // 3) * 3 + c // 3
                for d in range(9):
                    first = node
                    for column in (1 + r * 9 + c, 82 + r * 9 + d, 163 + c * 9 + d, 244 + box * 9 + d):
                        self.col[node] = column
                        self.row[node] = (r * 9 + c) * 9 + d
                        self.up[node] = self.up[column]
                        self.down[node] = column
                        self.down[self.up[column]] = node
                        self.up[column] = node
                        self.count[column] += 1
                        self.left[node] = node - 1
                        self.right[node] = node + 1
                        node += 1
                    self.left[first] = node - 1
                    self.right[node - 1] = first
        self.solutions = 0
        self.tries = 0
        self.chosen = [0] * 81
        self.first: list[int] = []

    def cover(self, c: int) -> None:
        left, right, up, down = self.left, self.right, self.up, self.down
        right[left[c]] = right[c]
        left[right[c]] = left[c]
        i = down[c]
        while i != c:
            j = right[i]
            while j != i:
                down[up[j]] = down[j]
                up[down[j]] = up[j]
                self.count[self.col[j]] -= 1
                j = right[j]
            i = down[i]

    def uncover(self, c: int) -> None:
        left, right, up, down = self.left, self.right, self.up, self.down
        i = up[c]
        while i != c:
            j = left[i]
            while j != i:
                self.count[self.col[j]] += 1
                down[up[j]] = j
                up[down[j]] = j
                j = left[j]
            i = up[i]
        right[left[c]] = c
        left[right[c]] = c

    def choose_row(self, node: int) -> None:
        j = self.right[node]
        while j != node:
            self.cover(self.col[j])
            j = self.right[j]

    def unchoose_row(self, node: int) -> None:
        j = self.left[node]
        while j != node:
            self.uncover(self.col[j])
            j = self.left[j]

    def search(self, depth: int) -> None:
        right = self.right
        if right[0] == 0:
            self.solutions += 1
            if self.solutions == 1:
                self.first = self.chosen[:depth]
            return
        best = right[0]
        c = right[best]
        while c != 0:
            if self.count[c] < self.count[best]:
                best = c
            c = right[c]
        self.cover(best)
        r = self.down[best]
        while r != best:
            self.tries += 1
            self.chosen[depth] = self.row[r]
            self.choose_row(r)
            self.search(depth + 1)
            self.unchoose_row(r)
            r = self.down[r]
        self.uncover(best)


def sudoku(puzzles: list[list[int]]) -> str:
    solutions = tries = 0
    h = 0
    for puzzle in puzzles:
        dlx = Dlx()
        depth = 0
        for cell, digit in enumerate(puzzle):
            if digit:
                node = DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4
                dlx.cover(dlx.col[node])
                dlx.choose_row(node)
                dlx.chosen[depth] = cell * 9 + digit - 1
                depth += 1
        dlx.search(depth)
        solutions += dlx.solutions
        tries += dlx.tries
        grid = [0] * 81
        for row in dlx.first:
            grid[row // 9] = row % 9 + 1
        h = h31([h, *grid])
    return f"{len(puzzles)}:{solutions}:{tries}:{h}"
