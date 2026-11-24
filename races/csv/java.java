// Race: csv (java)
// Extracted from suite/java/src/main/java/kernels/Work.java — build and run via suite/run.py

package kernels;
import com.github.luben.zstd.Zstd;
import de.siegmar.fastcsv.reader.CsvReader;
import java.awt.RenderingHints;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HexFormat;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.function.Function;
import java.util.function.UnaryOperator;
import java.util.stream.Collectors;
import java.util.zip.GZIPInputStream;
import java.util.zip.GZIPOutputStream;
import javax.imageio.IIOImage;
import javax.imageio.ImageIO;
import javax.imageio.ImageReader;
import javax.imageio.ImageWriteParam;
import javax.imageio.ImageWriter;
import javax.imageio.stream.MemoryCacheImageOutputStream;
import net.coobird.thumbnailator.Thumbnails;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVRecord;
import org.springframework.security.crypto.argon2.Argon2PasswordEncoder;
import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;

import java.io.IOException;

import java.io.UncheckedIOException;

import java.nio.file.Files;

import java.nio.file.Path;

import java.util.Comparator;

import java.util.HashMap;

import java.util.Map;

import java.util.function.Function;

import java.util.function.UnaryOperator;

import org.apache.commons.csv.CSVFormat;

import org.apache.commons.csv.CSVRecord;

final class Work {

  private static <I, R> Main.Kernel<I, R> kernel(
      Main.Prepare<I> prepare,
      Function<I, R> run,
      Function<R, String> checksum,
      Function<R, Map<String, Long>> extra) {
    return new Main.Kernel<>(
        prepare, UnaryOperator.identity(), (in, ctx) -> run.apply(in), checksum, extra);
  }

  private static <I, R> Main.Kernel<I, R> kernel(
      Main.Prepare<I> prepare, Function<I, R> run, Function<R, String> checksum) {
    return kernel(prepare, run, checksum, r -> Map.of());
  }

  @FunctionalInterface
  interface FileWork<R> {
    R run(Path path) throws IOException;
  }

  private static <R> Main.Kernel<Path, R> fileKernel(
      String pattern, FileWork<R> work, Function<R, String> checksum) {
    Function<Path, R> run =
        path -> {
          try {
            return work.run(path);
          } catch (IOException e) {
            throw new UncheckedIOException(e);
          }
        };
    return kernel((n, data) -> data.resolve(pattern.formatted(n)), run, checksum);
  }

  record Totals(long rows, Map<Integer, Long> sums) {}

  private static Totals commonsCsv(Path path) throws IOException {
    var sums = new HashMap<Integer, Long>();
    long rows = 0;
    var format = CSVFormat.DEFAULT.builder().setHeader().setSkipHeaderRecord(true).get();
    try (var reader = Files.newBufferedReader(path);
        var parser = format.parse(reader)) {
      for (CSVRecord order : parser) {
        rows++;
        sums.merge(
            Integer.parseInt(order.get("customer_id")),
            Long.parseLong(order.get("total_cents")),
            Long::sum);
      }
    }
    return new Totals(rows, sums);
  }

  private static String csvChecksum(Totals t) {
    long sum = t.sums().values().stream().mapToLong(Long::longValue).sum();
    var top =
        t.sums().entrySet().stream()
            .min(
                Comparator.comparing(Map.Entry<Integer, Long>::getValue)
                    .reversed()
                    .thenComparing(Map.Entry::getKey))
            .orElseThrow();
    return "%d:%d:%d:%d:%d".formatted(t.rows(), t.sums().size(), sum, top.getKey(), top.getValue());
  }

}
