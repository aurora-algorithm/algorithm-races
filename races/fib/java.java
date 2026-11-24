// Race: fib (java)
// Extracted from suite/java/src/main/java/kernels/Algo.java — build and run via suite/run.py

package kernels;
import java.util.Arrays;
import java.util.Map;

final class Algo {

  static long fib(int n) {
    return n < 2 ? n : fib(n - 1) + fib(n - 2);
  }

}
