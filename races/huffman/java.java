// Race: huffman (java)
// Extracted from suite/java/src/main/java/kernels/PuzzleText.java — build and run via suite/run.py

package kernels;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.PriorityQueue;

import java.util.ArrayDeque;

import java.util.Arrays;

import java.util.PriorityQueue;

final class PuzzleText {

  private static final int ONE_MIB = 1 << 20;

  /** A 1 MiB block of letters skewed toward {@code a} (two generator values each), repeated. */
  static byte[] skewedText(int n) {
    var rng = new Gen.XorShift32();
    var block = new byte[ONE_MIB];
    for (int i = 0; i < ONE_MIB; i++) {
      long a = rng.next() % 26;
      long b = rng.next() % 26;
      block[i] = (byte) (97 + a * b / 26);
    }
    var text = new byte[n / ONE_MIB * ONE_MIB];
    for (int offset = 0; offset < text.length; offset += ONE_MIB) {
      System.arraycopy(block, 0, text, offset, ONE_MIB);
    }
    return text;
  }

  record HuffmanResult(int n, long bits, byte[] encoded, boolean ok) {}

  static HuffmanResult huffman(byte[] text) {
    var freq = new long[256];
    for (byte b : text) {
      freq[b & 0xFF]++;
    }
    var heap = new PriorityQueue<Long>();
    for (int sym = 0; sym < 256; sym++) {
      if (freq[sym] > 0) {
        heap.add(freq[sym] * 1024 + sym);
      }
    }
    var left = new int[512];
    var right = new int[512];
    int nextId = 256;
    while (heap.size() > 1) {
      long a = heap.poll();
      long b = heap.poll();
      left[nextId] = (int) (a % 1024);
      right[nextId] = (int) (b % 1024);
      heap.add((a / 1024 + b / 1024) * 1024 + nextId);
      nextId++;
    }
    int root = (int) (heap.peek() % 1024);
    var codes = new int[256];
    var lengths = new int[256];
    var stack = new ArrayDeque<int[]>();
    stack.push(new int[] {root, 0, 0});
    while (!stack.isEmpty()) {
      int[] top = stack.pop();
      int node = top[0];
      if (node < 256) {
        codes[node] = top[1];
        lengths[node] = top[2];
      } else {
        stack.push(new int[] {left[node], top[1] << 1, top[2] + 1});
        stack.push(new int[] {right[node], (top[1] << 1) | 1, top[2] + 1});
      }
    }
    long capacity = 0;
    for (int sym = 0; sym < 256; sym++) {
      capacity += freq[sym] * lengths[sym];
    }
    var encoded = new byte[(int) ((capacity + 7) / 8)];
    int written = 0;
    long acc = 0;
    int nbits = 0;
    long bits = 0;
    for (byte value : text) {
      int b = value & 0xFF;
      acc = (acc << lengths[b]) | codes[b];
      nbits += lengths[b];
      bits += lengths[b];
      while (nbits >= 8) {
        nbits -= 8;
        encoded[written++] = (byte) (acc >> nbits);
      }
      acc &= (1L << nbits) - 1;
    }
    if (nbits > 0) {
      encoded[written++] = (byte) (acc << (8 - nbits));
    }
    var decoded = new byte[text.length];
    int node = root;
    int k = 0;
    for (int i = 0; i < written && k < text.length; i++) {
      int byteValue = encoded[i] & 0xFF;
      for (int shift = 7; shift >= 0 && k < text.length; shift--) {
        node = ((byteValue >> shift) & 1) != 0 ? right[node] : left[node];
        if (node < 256) {
          decoded[k++] = (byte) node;
          node = root;
        }
      }
    }
    return new HuffmanResult(text.length, bits, encoded, Arrays.equals(decoded, text));
  }

  static String huffmanChecksum(HuffmanResult r) {
    return r.n() + ":" + r.bits() + ":" + Gen.fnv1a(r.encoded()) + ":" + (r.ok() ? 1 : 0);
  }

}
