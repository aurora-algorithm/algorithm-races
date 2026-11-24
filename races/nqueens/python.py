# Race: nqueens (python)
# Extracted from suite/python/algo.py — build and run via suite/run.py

def nqueens(n: int) -> str:
    cols = [False] * n
    up = [False] * (2 * n - 1)
    down = [False] * (2 * n - 1)

    def place(row: int) -> int:
        if row == n:
            return 1
        count = 0
        for col in range(n):
            if not cols[col] and not up[row + col] and not down[row - col + n - 1]:
                cols[col] = up[row + col] = down[row - col + n - 1] = True
                count += place(row + 1)
                cols[col] = up[row + col] = down[row - col + n - 1] = False
        return count

    return str(place(0))
