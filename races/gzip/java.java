// Race: gzip (java)
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

import java.io.ByteArrayInputStream;

import java.io.ByteArrayOutputStream;

import java.io.IOException;

import java.io.UncheckedIOException;

import java.nio.file.Files;

import java.nio.file.Path;

import java.util.Map;

import java.util.function.Function;

import java.util.function.UnaryOperator;

import java.util.zip.GZIPInputStream;

import java.util.zip.GZIPOutputStream;

final class Work {

  private Work() {}

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

  private static byte[] corpus(int n, Path data) throws IOException {
    return Files.readAllBytes(data.resolve("corpus-" + n + ".bin"));
  }

  private static Main.Kernel<byte[], byte[]> compressKernel(
      UnaryOperator<byte[]> compress, UnaryOperator<byte[]> decompress) {
    return kernel(
        Work::corpus,
        compress,
        packed -> plainChecksum(decompress.apply(packed)),
        packed -> Map.of("compressed_bytes", (long) packed.length));
  }

  /** GZIPOutputStream has no level argument; its default (zlib's -1) is level 6. */
  private static byte[] gzip(byte[] raw) {
    var out = new ByteArrayOutputStream();
    try (var gzip = new GZIPOutputStream(out)) {
      gzip.write(raw);
    } catch (IOException e) {
      throw new UncheckedIOException(e);
    }
    return out.toByteArray();
  }

  private static byte[] gunzip(byte[] packed) {
    try (var gzip = new GZIPInputStream(new ByteArrayInputStream(packed))) {
      return gzip.readAllBytes();
    } catch (IOException e) {
      throw new UncheckedIOException(e);
    }
  }

}
