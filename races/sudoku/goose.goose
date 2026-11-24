// Race: sudoku (goose)
// Extracted from suite/goose/sudoku.goose — build and run via suite/run.py

// The `sudoku` kernel of the puzzle group (kernels/SPEC.md section 13): Knuth's Algorithm X with dancing links.

import gen;

let SUDOKU_CLUES = 26;
let DLX_COLUMNS = 324;
let DLX_SIZE = 1 + 324 + 729 * 4;

fn shuffle_with(r: XorShift32&, items: i64[:]) {
    var i = items.len - 1;
    while i > 0 {
        let j = r.next() % (i + 1);
        let t = items[i];
        items[i] = items[j];
        items[j] = t;
        i--;
    }
}

fn shuffled_lines(r: XorShift32&) -> i64[9] {
    var bands: i64[3] = [0, 1, 2];
    shuffle_with(r, bands);
    var lines: i64[9] = [0; 9];
    for band, b in bands {
        var inner: i64[3] = [0, 1, 2];
        shuffle_with(r, inner);
        for x, i in inner { lines[b * 3 + i] = band * 3 + x; }
    }
    return lines;
}

fn sudoku_puzzles(n: i64) -> i64[81][>..] {
    var r = xorshift();
    var puzzles: i64[81][>..] = [];
    for p in n {
        var digits: i64[9] = [1, 2, 3, 4, 5, 6, 7, 8, 9];
        shuffle_with(r, digits);
        let rows = shuffled_lines(r);
        let cols = shuffled_lines(r);
        var cells: i64[81] = [0; 81];
        for i in 81 { cells[i] = i; }
        shuffle_with(r, cells);
        let puzzle .= puzzles.push([0; 81]);
        for cell in cells[..SUDOKU_CLUES] {
            let row = rows[cell / 9];
            let col = cols[cell % 9];
            puzzle[cell] = digits[(row * 3 + row / 3 + col) % 9];
        }
    }
    return puzzles;
}

// The exact-cover matrix as flat arrays: node 0 is the root, 1..324 the column headers, then 4 nodes per row.
struct Dlx {
    left:      i32[DLX_SIZE],
    right:     i32[DLX_SIZE],
    up:        i32[DLX_SIZE],
    down:      i32[DLX_SIZE],
    col:       i32[DLX_SIZE],
    row:       i32[DLX_SIZE],
    count:     i32[DLX_COLUMNS + 1],
    solutions: i64,
    tries:     i64,
    chosen:    i32[81],
    first:     i32[81],
}

fn dlx_init(d: Dlx&) {
    for i in DLX_SIZE {
        d.up[i] = i as! i32;
        d.down[i] = i as! i32;
        d.col[i] = i as! i32;
        d.row[i] = 0;
    }
    for i in DLX_COLUMNS + 1 {
        d.left[i] = (i - 1) as! i32;
        d.right[i] = (i + 1) as! i32;
        d.count[i] = 0;
    }
    d.left[0] = DLX_COLUMNS;
    d.right[DLX_COLUMNS] = 0;
    var node = DLX_COLUMNS + 1;
    for r in 9 {
        for c in 9 {
            let box = (r / 3) * 3 + c / 3;
            for digit in 9 {
                let first = node;
                for column in [1 + r * 9 + c, 82 + r * 9 + digit, 163 + c * 9 + digit, 244 + box * 9 + digit] {
                    d.col[node] = column as! i32;
                    d.row[node] = ((r * 9 + c) * 9 + digit) as! i32;
                    d.up[node] = d.up[column];
                    d.down[node] = column as! i32;
                    d.down[d.up[column]] = node as! i32;
                    d.up[column] = node as! i32;
                    d.count[column]++;
                    d.left[node] = (node - 1) as! i32;
                    d.right[node] = (node + 1) as! i32;
                    node++;
                }
                d.left[first] = (node - 1) as! i32;
                d.right[node - 1] = first as! i32;
            }
        }
    }
    d.solutions = 0;
    d.tries = 0;
}

fn cover(d: Dlx&, c: i32) {
    d.right[d.left[c]] = d.right[c];
    d.left[d.right[c]] = d.left[c];
    var i = d.down[c];
    while i != c {
        var j = d.right[i];
        while j != i {
            d.down[d.up[j]] = d.down[j];
            d.up[d.down[j]] = d.up[j];
            d.count[d.col[j]]--;
            j = d.right[j];
        }
        i = d.down[i];
    }
}

fn uncover(d: Dlx&, c: i32) {
    var i = d.up[c];
    while i != c {
        var j = d.left[i];
        while j != i {
            d.count[d.col[j]]++;
            d.down[d.up[j]] = j;
            d.up[d.down[j]] = j;
            j = d.left[j];
        }
        i = d.up[i];
    }
    d.right[d.left[c]] = c;
    d.left[d.right[c]] = c;
}

fn choose_row(d: Dlx&, node: i32) {
    var j = d.right[node];
    while j != node {
        cover(d, d.col[j]);
        j = d.right[j];
    }
}

fn unchoose_row(d: Dlx&, node: i32) {
    var j = d.left[node];
    while j != node {
        uncover(d, d.col[j]);
        j = d.left[j];
    }
}

recursive fn search(d: Dlx&, depth: i64) {
    if d.right[0] == 0 {
        d.solutions++;
        if d.solutions == 1 { d.first = d.chosen; }
        return;
    }
    var best = d.right[0];
    var c = d.right[best];
    while c != 0 {
        if d.count[c] < d.count[best] { best = c; }
        c = d.right[c];
    }
    cover(d, best);
    var r = d.down[best];
    while r != best {
        d.tries++;
        d.chosen[depth] = d.row[r];
        choose_row(d, r);
        search(d, depth + 1);
        unchoose_row(d, r);
        r = d.down[r];
    }
    uncover(d, best);
}

fn sudoku(puzzles: i64[81][:], d: Dlx&, out: u8[>..]&) {
    var solutions = 0;
    var tries = 0;
    var h: u32 = 0;
    for puzzle in puzzles {
        dlx_init(d);
        var depth = 0;
        for digit, cell in puzzle {
            if digit == 0 { continue; }
            let node = (DLX_COLUMNS + 1 + (cell * 9 + digit - 1) * 4) as! i32;
            cover(d, d.col[node]);
            choose_row(d, node);
            d.chosen[depth] = (cell * 9 + digit - 1) as! i32;
            depth++;
        }
        search(d, depth);
        solutions += d.solutions;
        tries += d.tries;
        var grid: i64[81] = [0; 81];
        for row in d.first { grid[row / 9] = row % 9 + 1; }
        for g in grid { h = h31_step(h, g); }
    }
    format(out, puzzles.len, ":", solutions, ":", tries, ":", h);
}
