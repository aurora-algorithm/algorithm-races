// Race: bcrypt (java)
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

import java.util.List;

import org.springframework.security.crypto.bcrypt.BCryptPasswordEncoder;

final class Work {

  private static final int BCRYPT_COST = 10;

  private static final String REF_PASSWORD = "correct horse battery staple";

  private static final String BCRYPT_REF =
      "$2b$10$stacksbenchreferenceseTQ0n1pmWVgiwnlMiYJX0zumr35OiLyy";

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

}
