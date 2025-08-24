#pragma once

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iconv.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <uchardet.h>
#include <vector>

namespace fs = std::filesystem;

/**
 * @enum ConversionResult
 * @brief Result of file conversion operation
 */
enum class ConversionResult
{
    Success = 0,                ///< Conversion successful
    EmptyFile = 1,              ///< File is empty
    AlreadyTargetEncoding = 2,  ///< File is already in target encoding
    CannotDetectEncoding = 3,   ///< Cannot detect file encoding
    BackupFailed = 4,           ///< Failed to create backup
    ConversionFailed = 5,       ///< Conversion failed
    LibraryFailure = 6          ///< Library error
};

/**
 * @brief Convert ConversionResult enum to human-readable string.
 *
 * @param result The ConversionResult to convert.
 * @return std::string Human-readable description of the result.
 */
static std::string conversionResultToString(ConversionResult result)
{
    switch (result)
    {
    case ConversionResult::Success:
        return "Conversion successful";
    case ConversionResult::EmptyFile:
        return "File is empty";
    case ConversionResult::AlreadyTargetEncoding:
        return "File is already in target encoding";
    case ConversionResult::CannotDetectEncoding:
        return "Cannot detect file encoding";
    case ConversionResult::BackupFailed:
        return "Failed to create backup file";
    case ConversionResult::ConversionFailed:
        return "Conversion failed";
    case ConversionResult::LibraryFailure:
        return "Library error";
    default:
        return "Unknown error";
    }
}

/**
 * @struct ConversionInfo
 * @brief Detailed information about file conversion
 */
struct ConversionInfo
{
    ConversionResult result;
    std::string sourceEncoding;
    std::string targetEncoding;
    std::string errorMessage;

    ConversionInfo(ConversionResult res, const std::string &src = "", const std::string &target = "", const std::string &error = "")
        : result(res)
        , sourceEncoding(src)
        , targetEncoding(target)
        , errorMessage(error)
    {
    }
};

/**
 * @class FileConverter
 * @brief A utility class for batch detecting and converting file encodings.
 *
 * This class uses uchardet for encoding detection and libiconv for conversion, providing a simple interface
 * to process files in specified directories. All methods are static, no need to instantiate the class.
 */
class FileConverter
{
public:
    /**
     * @brief Convert encoding of a single file with detailed information.
     *
     * @param filepath Path to the file to convert.
     * @param target_encoding Target encoding, e.g. "UTF-8".
     * @param backup_enabled Whether to create a backup file before conversion.
     * @return ConversionInfo containing detailed conversion information.
     */
    static ConversionInfo convertFileWithInfo(const fs::path &filepath, const std::string &target_encoding, bool backup_enabled = false)
    {
        try
        {
            // Create backup if enabled
            if (backup_enabled)
            {
                try
                {
                    createBackupFile(filepath);
                }
                catch (const std::exception &e)
                {
                    return ConversionInfo(ConversionResult::BackupFailed, "", target_encoding, e.what());
                }
            }

            // 1. Read file content once
            std::vector<char> file_bytes = readFileAsBytes(filepath);
            if (file_bytes.empty())
            {
                return ConversionInfo(ConversionResult::EmptyFile, "", target_encoding);
            }

            // 2. Detect file encoding from buffer
            std::string source_encoding = detectFileEncodingFromBuffer(file_bytes);
            if (source_encoding.empty())
            {
                return ConversionInfo(ConversionResult::CannotDetectEncoding, "Unknown", target_encoding);
            }

            // 3. Check if conversion is needed
            if (source_encoding == target_encoding)
            {
                return ConversionInfo(ConversionResult::AlreadyTargetEncoding, source_encoding, target_encoding);
            }

            // 4. Convert encoding
            std::string converted_content;
            if (!convertEncoding(file_bytes, source_encoding, target_encoding, converted_content))
            {
                return ConversionInfo(ConversionResult::ConversionFailed, source_encoding, target_encoding, "Encoding conversion failed");
            }

            // 5. Write file (BOM will be added automatically if target is UTF-8-BOM)
            if (!writeFile(filepath, converted_content, target_encoding))
            {
                return ConversionInfo(ConversionResult::ConversionFailed, source_encoding, target_encoding, "Failed to write file");
            }

            return ConversionInfo(ConversionResult::Success, source_encoding, target_encoding);
        }
        catch (const std::exception &e)
        {
            return ConversionInfo(ConversionResult::ConversionFailed, "", target_encoding, e.what());
        }
        catch (...)
        {
            return ConversionInfo(ConversionResult::ConversionFailed, "", target_encoding, "Unknown error");
        }
    }

    /**
     * @brief Convert encoding of a single file.
     *
     * @param filepath Path to the file to convert.
     * @param target_encoding Target encoding, e.g. "UTF-8".
     * @param backup_enabled Whether to create a backup file before conversion.
     * @return ConversionResult indicating the operation status.
     */
    static ConversionResult convertFile(const fs::path &filepath, const std::string &target_encoding, bool backup_enabled = false)
    {
        ConversionInfo info = convertFileWithInfo(filepath, target_encoding, backup_enabled);
        return info.result;
    }

    /**
     * @brief Batch process files in specified directories and convert their encodings.
     *
     * @param target_dirs Vector containing all directory paths to process.
     * @param file_exts Vector containing all file extensions to convert, e.g. {".txt", ".cpp"}.
     * @param target_encoding Target encoding, e.g. "UTF-8".
     * @param backup_enabled Whether to create backup files before conversion.
     */
    static void processDirectory(
        const std::vector<std::string> &target_dirs, const std::vector<std::string> &file_exts, const std::string &target_encoding, bool backup_enabled = false)
    {

        std::cout << "Starting conversion process..." << std::endl;
        std::cout << "  Target Encoding: " << target_encoding << std::endl;

        for (const auto &target_dir : target_dirs)
        {
            fs::path dir_path = target_dir;
            if (!fs::exists(dir_path))
            {
                throw std::runtime_error("Directory does not exist: " + dir_path.string());
            }
            std::cout << "Processing directory: " << target_dir << std::endl;

            for (const auto &entry : fs::recursive_directory_iterator(dir_path))
            {
                if (entry.is_regular_file())
                {
                    std::string current_ext = entry.path().extension().string();
                    // Check if file extension is in target list
                    bool extension_match = std::find(file_exts.begin(), file_exts.end(), current_ext) != file_exts.end();

                    if (extension_match)
                    {
                        ConversionResult result = convertFile(entry.path(), target_encoding, backup_enabled);
                        if (result != ConversionResult::Success && result != ConversionResult::EmptyFile && result != ConversionResult::AlreadyTargetEncoding)
                        {
                            std::string error_msg;
                            switch (result)
                            {
                            case ConversionResult::CannotDetectEncoding:
                                error_msg = "Cannot detect encoding";
                                break;
                            case ConversionResult::BackupFailed:
                                error_msg = "Failed to create backup file";
                                break;
                            case ConversionResult::ConversionFailed:
                                error_msg = "Conversion failed";
                                break;
                            default:
                                error_msg = "Unknown error";
                                break;
                            }
                            std::cerr << "Error processing file " << entry.path() << ": " << error_msg << std::endl;
                        }
                    }
                }
            }
        }

        std::cout << "Conversion process finished." << std::endl;
    }

private:
    // Helper function: create backup file
    static void createBackupFile(const fs::path &filepath)
    {
        fs::path backup_path = filepath.string() + ".bak";
        fs::copy_file(filepath, backup_path, fs::copy_options::overwrite_existing);
    }

    // Helper function: read file content into a byte vector
    static std::vector<char> readFileAsBytes(const fs::path &filepath)
    {
        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file.");
        }
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(static_cast<size_t>(size));
        file.read(buffer.data(), size);
        return buffer;
    }

    // Helper function: check if buffer has UTF-8 BOM
    static bool hasUtf8Bom(const std::vector<char> &buffer)
    {
        return buffer.size() >= 3 && 
               static_cast<unsigned char>(buffer[0]) == 0xEF &&
               static_cast<unsigned char>(buffer[1]) == 0xBB &&
               static_cast<unsigned char>(buffer[2]) == 0xBF;
    }

    // Helper function: detect file encoding from buffer using uchardet and BOM detection
    static std::string detectFileEncodingFromBuffer(const std::vector<char> &buffer)
    {
        if (buffer.empty())
        {
            return "";
        }

        // Check for UTF-16 BOM first (these are different encodings)
        if (buffer.size() >= 2)
        {
            if (static_cast<unsigned char>(buffer[0]) == 0xFF && static_cast<unsigned char>(buffer[1]) == 0xFE)
            {
                return "UTF-16LE";
            }
            if (static_cast<unsigned char>(buffer[0]) == 0xFE && static_cast<unsigned char>(buffer[1]) == 0xFF)
            {
                return "UTF-16BE";
            }
        }

        // Use uchardet for encoding detection
        uchardet_t detector = uchardet_new();
        if (!detector)
        {
            throw std::runtime_error("Failed to create uchardet detector.");
        }

        // Skip UTF-8 BOM for detection if present
        const char* detect_data = buffer.data();
        size_t detect_size = buffer.size();
        if (hasUtf8Bom(buffer))
        {
            detect_data += 3;
            detect_size -= 3;
        }

        uchardet_handle_data(detector, detect_data, detect_size);
        uchardet_data_end(detector);

        const char *charset = uchardet_get_charset(detector);
        std::string result = charset ? charset : "";
        uchardet_delete(detector);

        // Map common encoding names
        if (result == "UTF-8" || result == "ASCII" || result.empty())
        {
            // ASCII is a subset of UTF-8, so treat ASCII with BOM as UTF-8-BOM
            return hasUtf8Bom(buffer) ? "UTF-8-BOM" : "UTF-8";
        }
        else if (result == "GB18030" || result == "GBK")
        {
            return "GBK";
        }

        return result;
    }

    // Helper function: detect file encoding using uchardet and BOM detection (wrapper for compatibility)
    static std::string detectFileEncoding(const fs::path &filepath)
    {
        std::vector<char> buffer = readFileAsBytes(filepath);
        return detectFileEncodingFromBuffer(buffer);
    }

    // Helper function: get base encoding (remove BOM suffix)
    static std::string getBaseEncoding(const std::string &encoding)
    {
        if (encoding == "UTF-8-BOM") return "UTF-8";
        return encoding;
    }

    // Helper function: check if encoding should have BOM
    static bool shouldHaveBom(const std::string &encoding)
    {
        return encoding == "UTF-8-BOM";
    }

    // Helper function: convert encoding using iconv
    static bool convertEncoding(const std::vector<char> &input_data, const std::string &from_encoding, const std::string &to_encoding, std::string &output_data)
    {
        // Get base encodings for iconv
        std::string iconv_from = getBaseEncoding(from_encoding);
        std::string iconv_to = getBaseEncoding(to_encoding);

        // Prepare input data (skip BOM if present in source)
        const char *input_ptr = input_data.data();
        size_t input_size = input_data.size();

        // Skip UTF-8 BOM if present in source
        if (from_encoding == "UTF-8-BOM" && hasUtf8Bom(input_data))
        {
            input_ptr += 3;
            input_size -= 3;
        }

        iconv_t cd = iconv_open(iconv_to.c_str(), iconv_from.c_str());
        if (cd == (iconv_t)-1)
        {
            return false;
        }

        char *in_buf = const_cast<char *>(input_ptr);
        size_t in_bytes_left = input_size;

        // Set a reasonable output buffer size
        size_t out_buf_size = in_bytes_left * 2 + 100;
        std::vector<char> out_buffer(out_buf_size);
        char *out_buf = out_buffer.data();
        size_t out_bytes_left = out_buf_size;

        size_t result = iconv(cd, &in_buf, &in_bytes_left, &out_buf, &out_bytes_left);
        if (result == (size_t)-1)
        {
            iconv_close(cd);
            return false;
        }

        output_data.assign(out_buffer.data(), out_buf_size - out_bytes_left);
        iconv_close(cd);
        return true;
    }

    // Helper function: write data to file
    static bool writeFile(const fs::path &filepath, const std::string &content, const std::string &encoding = "UTF-8")
    {
        std::ofstream file(filepath, std::ios::binary | std::ios::trunc);
        if (!file.is_open())
        {
            return false;
        }

        // Add BOM if needed
        if (shouldHaveBom(encoding))
        {
            const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
            file.write(reinterpret_cast<const char *>(bom), 3);
        }

        file.write(content.c_str(), content.length());
        file.close();
        return true;
    }
};
