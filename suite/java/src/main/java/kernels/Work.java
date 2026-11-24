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

/**
 * The "work" kernels (kernels/SPEC.md section 11): images, compression, passwords, CSV, word count.
 * Main loads this class by name, so native-image never compiles AWT or the JNI libraries.
 */
final class Work {
  private static final int PHOTOS = 8;
  private static final int TARGET_WIDTH = 1080;
  private static final float JPEG_QUALITY = 0.8f;
  private static final int BCRYPT_COST = 10;
  private static final int ARGON2_MEMORY = 19456;
  private static final int ARGON2_TIME = 2;
  private static final int ARGON2_LANES = 1;
  private static final String ARGON2_PREFIX = "$argon2id$v=19$m=19456,t=2,p=1$";
  private static final String REF_PASSWORD = "correct horse battery staple";
  private static final String BCRYPT_REF =
      "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";
  private static final String ARGON2_REF =
      "$argon2id$v=19$m=19456,t=2,p=1$c3RhY2tzLWJlbmNoLXNhbHQ$oLeQRnXjHhLMJZcTdYDeVr93DW7srzPjTWhPG3BTryk";

  private Work() {}

  static Map<String, Main.Kernel<?, ?>> kernels() {
    System.setProperty("java.awt.headless", "true");
    // ImageIO's stream cache defaults to temp files on disk; the other languages encode to memory.
    ImageIO.setUseCache(false);
    return Map.ofEntries(
        Map.entry("image", imageKernel(Work::convertImageIo)),
        Map.entry("image-alt", imageKernel(Work::convertThumbnailator)),
        Map.entry("gzip", compressKernel(Work::gzip, Work::gunzip)),
        Map.entry("gunzip", decompressKernel(Work::gzip, Work::gunzip)),
        Map.entry("zstd", compressKernel(Work::zstd, Work::unzstd)),
        Map.entry("unzstd", decompressKernel(Work::zstd, Work::unzstd)),
        Map.entry("bcrypt", passwordKernel(Work::bcryptHash, Work::bcryptChecksum)),
        Map.entry("argon2id", passwordKernel(Work::argon2Hash, Work::argon2Checksum)),
        Map.entry("csv", fileKernel("orders-%d.csv", Work::commonsCsv, Work::csvChecksum)),
        Map.entry("csv-alt", fileKernel("orders-%d.csv", Work::fastCsv, Work::csvChecksum)),
        Map.entry(
            "wordcount", fileKernel("text-%d.txt", Work::wordcount, Work::wordcountChecksum)));
  }

  /** The pinned versions, written into work-libraries.properties by the Maven build. */
  static String libraries() {
    var props = new Properties();
    try (var in = Work.class.getResourceAsStream("/work-libraries.properties")) {
      if (in != null) {
        props.load(in);
      }
    } catch (IOException e) {
      throw new UncheckedIOException(e);
    }
    return props.stringPropertyNames().stream()
        .sorted()
        .map(name -> name + " " + props.getProperty(name))
        .collect(Collectors.joining(", "));
  }

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

  private static int targetHeight(int width, int height) {
    return (height * TARGET_WIDTH + width / 2) / width;
  }

  @FunctionalInterface
  interface Converter {
    byte[] convert(byte[] jpeg) throws IOException;
  }

  private static Main.Kernel<List<byte[]>, List<byte[]>> imageKernel(Converter converter) {
    Main.Prepare<List<byte[]>> load =
        (n, data) -> {
          var photos = new ArrayList<byte[]>();
          for (int i = 0; i < n; i++) {
            photos.add(Files.readAllBytes(data.resolve("photos/photo-" + (i % PHOTOS) + ".jpg")));
          }
          return photos;
        };
    Function<List<byte[]>, List<byte[]>> run =
        photos -> {
          var out = new ArrayList<byte[]>();
          for (var photo : photos) {
            try {
              out.add(converter.convert(photo));
            } catch (IOException e) {
              throw new UncheckedIOException(e);
            }
          }
          return out;
        };
    return kernel(
        load,
        run,
        out -> out.size() + ":" + jpegSize(out.getLast()),
        out -> Map.of("out_bytes", out.stream().mapToLong(b -> b.length).sum()));
  }

  /** "WxH" from the JPEG header, read after timing. */
  private static String jpegSize(byte[] jpeg) {
    try (var in = ImageIO.createImageInputStream(new ByteArrayInputStream(jpeg))) {
      ImageReader reader = ImageIO.getImageReaders(in).next();
      try {
        reader.setInput(in);
        return reader.getWidth(0) + "x" + reader.getHeight(0);
      } finally {
        reader.dispose();
      }
    } catch (IOException e) {
      throw new UncheckedIOException(e);
    }
  }

  private static byte[] convertImageIo(byte[] jpeg) throws IOException {
    BufferedImage src = ImageIO.read(new ByteArrayInputStream(jpeg));
    int height = targetHeight(src.getWidth(), src.getHeight());
    var small = new BufferedImage(TARGET_WIDTH, height, BufferedImage.TYPE_INT_RGB);
    var g = small.createGraphics();
    g.setRenderingHint(
        RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BICUBIC);
    g.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
    g.drawImage(src, 0, 0, TARGET_WIDTH, height, null);
    g.dispose();
    var out = new ByteArrayOutputStream();
    ImageWriter writer = ImageIO.getImageWritersByFormatName("jpeg").next();
    ImageWriteParam param = writer.getDefaultWriteParam();
    param.setCompressionMode(ImageWriteParam.MODE_EXPLICIT);
    param.setCompressionQuality(JPEG_QUALITY);
    try (var stream = new MemoryCacheImageOutputStream(out)) {
      writer.setOutput(stream);
      writer.write(null, new IIOImage(small, null, null), param);
    } finally {
      writer.dispose();
    }
    return out.toByteArray();
  }

  private static byte[] convertThumbnailator(byte[] jpeg) throws IOException {
    var out = new ByteArrayOutputStream();
    Thumbnails.of(new ByteArrayInputStream(jpeg))
        .width(TARGET_WIDTH)
        .outputFormat("jpg")
        .outputQuality(JPEG_QUALITY)
        .toOutputStream(out);
    return out.toByteArray();
  }

  private static byte[] corpus(int n, Path data) throws IOException {
    return Files.readAllBytes(data.resolve("corpus-" + n + ".bin"));
  }

  static String plainChecksum(byte[] raw) {
    try {
      var digest = MessageDigest.getInstance("SHA-256").digest(raw);
      return raw.length + ":" + HexFormat.of().formatHex(digest);
    } catch (NoSuchAlgorithmException e) {
      throw new IllegalStateException(e);
    }
  }

  private static Main.Kernel<byte[], byte[]> compressKernel(
      UnaryOperator<byte[]> compress, UnaryOperator<byte[]> decompress) {
    return kernel(
        Work::corpus,
        compress,
        packed -> plainChecksum(decompress.apply(packed)),
        packed -> Map.of("compressed_bytes", (long) packed.length));
  }

  private static Main.Kernel<byte[], byte[]> decompressKernel(
      UnaryOperator<byte[]> compress, UnaryOperator<byte[]> decompress) {
    return kernel((n, data) -> compress.apply(corpus(n, data)), decompress, Work::plainChecksum);
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

  private static byte[] zstd(byte[] raw) {
    return Zstd.compress(raw, 3);
  }

  private static byte[] unzstd(byte[] packed) {
    return Zstd.decompress(packed, Math.toIntExact(Zstd.getFrameContentSize(packed)));
  }

  private static List<String> passwords(int n, Path data) {
    var pws = new ArrayList<String>();
    for (int i = 0; i < n; i++) {
      pws.add("user-" + i + "-password");
    }
    return pws;
  }

  private static Main.Kernel<List<String>, List<String>> passwordKernel(
      Function<List<String>, List<String>> hash, Function<List<String>, String> checksum) {
    return kernel(Work::passwords, hash, checksum);
  }

  private static final BCryptPasswordEncoder BCRYPT = new BCryptPasswordEncoder(BCRYPT_COST);

  private static List<String> bcryptHash(List<String> pws) {
    return pws.stream().map(pw -> BCRYPT.encode(pw)).toList();
  }

  private static String bcryptChecksum(List<String> hashes) {
    int ok = 0;
    for (int i = 0; i < hashes.size(); i++) {
      var h = hashes.get(i);
      if (h.substring(3, 7).equals("$" + BCRYPT_COST + "$")
          && BCRYPT.matches("user-" + i + "-password", h)) {
        ok++;
      }
    }
    int ref = BCRYPT.matches(REF_PASSWORD, BCRYPT_REF) ? 1 : 0;
    return hashes.size() + ":" + ok + ":" + ref;
  }

  private static final Argon2PasswordEncoder ARGON2 =
      new Argon2PasswordEncoder(16, 32, ARGON2_LANES, ARGON2_MEMORY, ARGON2_TIME);

  private static List<String> argon2Hash(List<String> pws) {
    return pws.stream().map(pw -> ARGON2.encode(pw)).toList();
  }

  private static String argon2Checksum(List<String> hashes) {
    int ok = 0;
    for (int i = 0; i < hashes.size(); i++) {
      var h = hashes.get(i);
      if (h.startsWith(ARGON2_PREFIX) && ARGON2.matches("user-" + i + "-password", h)) {
        ok++;
      }
    }
    int ref = ARGON2.matches(REF_PASSWORD, ARGON2_REF) ? 1 : 0;
    return hashes.size() + ":" + ok + ":" + ref;
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

  private static Totals fastCsv(Path path) throws IOException {
    var sums = new HashMap<Integer, Long>();
    long rows = 0;
    try (var csv = CsvReader.builder().ofNamedCsvRecord(path)) {
      for (var order : csv) {
        rows++;
        sums.merge(
            Integer.parseInt(order.getField("customer_id")),
            Long.parseLong(order.getField("total_cents")),
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

  private static Map<String, Long> wordcount(Path path) throws IOException {
    var counts = new HashMap<String, Long>();
    try (var reader = Files.newBufferedReader(path)) {
      String line;
      while ((line = reader.readLine()) != null) {
        for (var word : line.split(" ")) {
          if (!word.isEmpty()) {
            counts.merge(word, 1L, Long::sum);
          }
        }
      }
    }
    return counts;
  }

  private static String wordcountChecksum(Map<String, Long> counts) {
    long total = counts.values().stream().mapToLong(Long::longValue).sum();
    var top =
        counts.entrySet().stream()
            .min(
                Comparator.comparing(Map.Entry<String, Long>::getValue)
                    .reversed()
                    .thenComparing(Map.Entry::getKey))
            .orElseThrow();
    return "%d:%d:%s:%d".formatted(total, counts.size(), top.getKey(), top.getValue());
  }
}
