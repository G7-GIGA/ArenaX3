#include "sys_save.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace com {
namespace arenax3 {

// --- Utility: Base64 (RFC 4648) ---
static const std::string base64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const unsigned char* bytes_to_encode, size_t in_len) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    size_t pos = 0;
    while (pos < in_len) {
        char_array_3[i++] = bytes_to_encode[pos++];
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            for (i = 0; i < 4; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    if (i) {
        for (j = i; j < 3; j++)
            char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++)
            ret += base64_chars[char_array_4[j]];
        while (i++ < 3)
            ret += '=';
    }
    return ret;
}

static std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    std::vector<unsigned char> ret;
    while (in_len-- && (encoded_string[in_] != '=') &&
           (isalnum(encoded_string[in_]) || (encoded_string[in_] == '+') || (encoded_string[in_] == '/'))) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<unsigned char>(base64_chars.find(char_array_4[i]));
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }
    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<unsigned char>(base64_chars.find(char_array_4[j]));
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        for (j = 0; j < i - 1; j++)
            ret.push_back(char_array_3[j]);
    }
    return ret;
}

// --- XOR obfuscation key ---
static const unsigned char xor_key[] = {0xA3, 0x7F, 0x2C, 0x91, 0x48, 0xEE, 0x15, 0x06};

static std::vector<unsigned char> obfuscate(const std::vector<unsigned char>& data) {
    std::vector<unsigned char> out(data.size());
    size_t key_len = sizeof(xor_key);
    for (size_t i = 0; i < data.size(); ++i) {
        out[i] = data[i] ^ xor_key[i % key_len];
    }
    return out;
}

// --- Secure hash (simple 64-bit FNV-1a with salt) ---
static uint64_t secure_hash(const std::string& input, uint32_t salt) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    std::string salted = std::to_string(salt) + input;
    for (unsigned char c : salted) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// --- CSV escape helper ---
static std::string escape_csv(const std::string& field) {
    if (field.find(',') != std::string::npos ||
        field.find('"') != std::string::npos ||
        field.find('\n') != std::string::npos) {
        std::string escaped = "\"";
        for (char c : field) {
            if (c == '"') escaped += "\"\"";
            else escaped += c;
        }
        escaped += "\"";
        return escaped;
    }
    return field;
}

static std::string unescape_csv(const std::string& field) {
    if (field.size() >= 2 && field.front() == '"' && field.back() == '"') {
        std::string inner = field.substr(1, field.size() - 2);
        std::string result;
        for (size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '"' && i + 1 < inner.size() && inner[i + 1] == '"') {
                result += '"';
                ++i;
            } else {
                result += inner[i];
            }
        }
        return result;
    }
    return field;
}

// --- Timestamp ---
static std::string current_iso8601() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%S");
    return oss.str();
}

// --- File paths ---
static std::filesystem::path save_directory() {
    // Platform-specific: user data directory
#ifdef _WIN32
    const char* appdata = std::getenv("APPDATA");
    std::filesystem::path base = appdata ? std::filesystem::path(appdata) : std::filesystem::current_path();
#else
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home ? std::filesystem::path(home) : std::filesystem::current_path();
#endif
    return base / "ArenaX3" / "saves";
}

static std::filesystem::path profile_path(const std::string& profile_name) {
    std::string safe_name = profile_name;
    std::replace(safe_name.begin(), safe_name.end(), '/', '_');
    std::replace(safe_name.begin(), safe_name.end(), '\\', '_');
    return save_directory() / (safe_name + ".ax3save");
}

// ========== SaveData implementation ==========

SaveData::SaveData() : version_(1), flags_(0) {}

void SaveData::set_player_name(const std::string& name) { player_name_ = name; }
std::string SaveData::player_name() const { return player_name_; }

void SaveData::set_level(int level) { level_ = level; }
int SaveData::level() const { return level_; }

void SaveData::set_health(float health) { health_ = health; }
float SaveData::health() const { return health_; }

void SaveData::set_position(float x, float y, float z) {
    pos_x_ = x; pos_y_ = y; pos_z_ = z;
}
float SaveData::pos_x() const { return pos_x_; }
float SaveData::pos_y() const { return pos_y_; }
float SaveData::pos_z() const { return pos_z_; }

void SaveData::set_inventory(const std::vector<InventoryItem>& inv) { inventory_ = inv; }
std::vector<InventoryItem> SaveData::inventory() const { return inventory_; }

void SaveData::set_flags(uint64_t flags) { flags_ = flags; }
uint64_t SaveData::flags() const { return flags_; }

void SaveData::set_quest_progress(const std::string& quest_id, int stage) {
    quest_progress_[quest_id] = stage;
}
int SaveData::quest_progress(const std::string& quest_id) const {
    auto it = quest_progress_.find(quest_id);
    return (it != quest_progress_.end()) ? it->second : 0;
}
std::unordered_map<std::string, int> SaveData::all_quest_progress() const { return quest_progress_; }

void SaveData::set_timestamp(const std::string& ts) { timestamp_ = ts; }
std::string SaveData::timestamp() const { return timestamp_; }

std::string SaveData::compute_checksum() const {
    std::ostringstream data;
    data << player_name_ << '|' << level_ << '|' << health_ << '|'
         << pos_x_ << ',' << pos_y_ << ',' << pos_z_ << '|';
    for (const auto& item : inventory_) {
        data << item.id << ':' << item.count << ';';
    }
    data << '|' << flags_ << '|';
    for (const auto& q : quest_progress_) {
        data << q.first << '=' << q.second << ';';
    }
    data << '|' << version_;
    uint64_t h = secure_hash(data.str(), 0xA3E7);
    std::ostringstream hex_stream;
    hex_stream << std::hex << std::setw(16) << std::setfill('0') << h;
    return hex_stream.str();
}

void SaveData::set_version(int v) { version_ = v; }
int SaveData::version() const { return version_; }

// ========== SysSave implementation ==========

SysSave::SysSave() : current_profile_("default"), auto_save_enabled_(true) {
    std::filesystem::create_directories(save_directory());
}

SysSave::~SysSave() {}

bool SysSave::save_profile(const std::string& profile_name, const SaveData& data) {
    try {
        SaveData write_data = data;
        write_data.set_timestamp(current_iso8601());
        std::string checksum = write_data.compute_checksum();

        std::ostringstream csv;
        // Header for obfuscated CSV
        csv << escape_csv(write_data.player_name()) << ","
            << write_data.level() << ","
            << write_data.health() << ","
            << write_data.pos_x() << ","
            << write_data.pos_y() << ","
            << write_data.pos_z() << ",";

        // Inventory: base64(id:count;id:count;...)
        std::ostringstream inv_stream;
        for (const auto& item : write_data.inventory()) {
            inv_stream << item.id << ":" << item.count << ";";
        }
        csv << escape_csv(base64_encode(
            reinterpret_cast<const unsigned char*>(inv_stream.str().data()),
            inv_stream.str().size())) << ",";

        csv << write_data.flags() << ",";

        // Quest progress: base64(key=stage;key=stage;...)
        std::ostringstream quest_stream;
        for (const auto& q : write_data.all_quest_progress()) {
            quest_stream << q.first << "=" << q.second << ";";
        }
        csv << escape_csv(base64_encode(
            reinterpret_cast<const unsigned char*>(quest_stream.str().data()),
            quest_stream.str().size())) << ",";

        csv << write_data.version() << ","
            << escape_csv(write_data.timestamp()) << ","
            << checksum;

        std::string csv_content = csv.str();
        std::vector<unsigned char> raw_bytes(csv_content.begin(), csv_content.end());
        std::vector<unsigned char> encrypted = obfuscate(raw_bytes);

        std::filesystem::path file_path = profile_path(profile_name);
        std::ofstream file(file_path, std::ios::binary);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(encrypted.data()), encrypted.size());
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool SysSave::load_profile(const std::string& profile_name, SaveData& out_data) {
    try {
        std::filesystem::path file_path = profile_path(profile_name);
        if (!std::filesystem::exists(file_path)) return false;

        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<unsigned char> encrypted(size);
        if (!file.read(reinterpret_cast<char*>(encrypted.data()), size)) return false;
        file.close();

        std::vector<unsigned char> decrypted = obfuscate(encrypted);
        std::string csv_content(decrypted.begin(), decrypted.end());

        // Parse CSV
        std::vector<std::string> fields;
        std::istringstream line_stream(csv_content);
        std::string field;
        bool in_quotes = false;
        std::string current_field;
        for (char c : csv_content) {
            if (in_quotes) {
                current_field += c;
                if (c == '"') {
                    in_quotes = false;
                }
            } else {
                if (c == '"') {
                    in_quotes = true;
                    current_field += c;
                } else if (c == ',') {
                    fields.push_back(unescape_csv(current_field));
                    current_field.clear();
                } else {
                    current_field += c;
                }
            }
        }
        fields.push_back(unescape_csv(current_field));

        if (fields.size() < 11) return false;

        out_data.set_player_name(fields[0]);
        out_data.set_level(std::stoi(fields[1]));
        out_data.set_health(std::stof(fields[2]));
        out_data.set_position(std::stof(fields[3]), std::stof(fields[4]), std::stof(fields[5]));

        // Decode inventory
        std::string inv_encoded = fields[6];
        std::vector<unsigned char> inv_raw = base64_decode(inv_encoded);
        std::string inv_str(inv_raw.begin(), inv_raw.end());
        std::vector<InventoryItem> inventory;
        std::istringstream inv_stream(inv_str);
        std::string item_pair;
        while (std::getline(inv_stream, item_pair, ';')) {
            if (item_pair.empty()) continue;
            auto colon_pos = item_pair.find(':');
            if (colon_pos != std::string::npos) {
                InventoryItem item;
                item.id = item_pair.substr(0, colon_pos);
                item.count = std::stoi(item_pair.substr(colon_pos + 1));
                inventory.push_back(item);
            }
        }
        out_data.set_inventory(inventory);

        out_data.set_flags(std::stoull(fields[7]));

        // Decode quests
        std::string quest_encoded = fields[8];
        std::vector<unsigned char> quest_raw = base64_decode(quest_encoded);
        std::string quest_str(quest_raw.begin(), quest_raw.end());
        std::istringstream quest_stream(quest_str);
        std::string quest_pair;
        while (std::getline(quest_stream, quest_pair, ';')) {
            if (quest_pair.empty()) continue;
            auto eq_pos = quest_pair.find('=');
            if (eq_pos != std::string::npos) {
                out_data.set_quest_progress(
                    quest_pair.substr(0, eq_pos),
                    std::stoi(quest_pair.substr(eq_pos + 1))
                );
            }
        }

        out_data.set_version(std::stoi(fields[9]));
        out_data.set_timestamp(fields[10]);
        std::string stored_checksum = fields[11];

        // Verify checksum
        std::string computed = out_data.compute_checksum();
        if (computed != stored_checksum) {
            // Checksum mismatch - data may be corrupted
            return false;
        }

        return true;
    } catch (...) {
        return false;
    }
}

bool SysSave::delete_profile(const std::string& profile_name) {
    try {
        std::filesystem::path file_path = profile_path(profile_name);
        if (std::filesystem::exists(file_path)) {
            return std::filesystem::remove(file_path);
        }
        return false;
    } catch (...) {
        return false;
    }
}

std::vector<std::string> SysSave::list_profiles() {
    std::vector<std::string> profiles;
    try {
        for (const auto& entry : std::filesystem::directory_iterator(save_directory())) {
            if (entry.is_regular_file() && entry.path().extension() == ".ax3save") {
                std::string filename = entry.path().stem().string();
                profiles.push_back(filename);
            }
        }
    } catch (...) {}
    std::sort(profiles.begin(), profiles.end());
    return profiles;
}

bool SysSave::profile_exists(const std::string& profile_name) {
    return std::filesystem::exists(profile_path(profile_name));
}

void SysSave::set_current_profile(const std::string& profile_name) {
    current_profile_ = profile_name;
}

std::string SysSave::current_profile() const {
    return current_profile_;
}

bool SysSave::quick_save(const SaveData& data) {
    if (!auto_save_enabled_) return false;
    return save_profile(current_profile_, data);
}

bool SysSave::quick_load(SaveData& out_data) {
    return load_profile(current_profile_, out_data);
}

void SysSave::enable_auto_save(bool enable) {
    auto_save_enabled_ = enable;
}

bool SysSave::is_auto_save_enabled() const {
    return auto_save_enabled_;
}

bool SysSave::export_save(const std::string& profile_name, const std::string& export_path) {
    SaveData data;
    if (!load_profile(profile_name, data)) return false;
    // Export as plain unencrypted JSON-like custom format
    try {
        std::ofstream file(export_path);
        if (!file.is_open()) return false;
        file << "player_name:" << data.player_name() << "\n";
        file << "level:" << data.level() << "\n";
        file << "health:" << data.health() << "\n";
        file << "position:" << data.pos_x() << "," << data.pos_y() << "," << data.pos_z() << "\n";
        file << "flags:" << data.flags() << "\n";
        file << "version:" << data.version() << "\n";
        file << "timestamp:" << data.timestamp() << "\n";
        file << "inventory:\n";
        for (const auto& item : data.inventory()) {
            file << "  - " << item.id << " x" << item.count << "\n";
        }
        file << "quests:\n";
        for (const auto& q : data.all_quest_progress()) {
            file << "  - " << q.first << " : " << q.second << "\n";
        }
        file.close();
        return true;
    } catch (...) {
        return false;
    }
}

bool SysSave::import_save(const std::string& profile_name, const std::string& import_path) {
    try {
        std::ifstream file(import_path);
        if (!file.is_open()) return false;

        SaveData data;
        std::string line;
        std::vector<InventoryItem> inventory;
        std::unordered_map<std::string, int> quests;
        bool in_inventory = false;
        bool in_quests = false;

        while (std::getline(file, line)) {
            if (line.empty()) continue;
            if (line == "inventory:") { in_inventory = true; in_quests = false; continue; }
            if (line == "quests:") { in_quests = true; in_inventory = false; continue; }

            if (in_inventory) {
                // Parse "  - id xcount"
                if (line.size() >= 4 && line.substr(0, 4) == "  - ") {
                    std::string content = line.substr(4);
                    auto x_pos = content.rfind(" x");
                    if (x_pos != std::string::npos) {
                        InventoryItem item;
                        item.id = content.substr(0, x_pos);
                        item.count = std::stoi(content.substr(x_pos + 2));
                        inventory.push_back(item);
                    }
                }
                continue;
            }
            if (in_quests) {
                if (line.size() >= 4 && line.substr(0, 4) == "  - ") {
                    std::string content = line.substr(4);
                    auto colon_pos = content.find(" : ");
                    if (colon_pos != std::string::npos) {
                        quests[content.substr(0, colon_pos)] = std::stoi(content.substr(colon_pos + 3));
                    }
                }
                continue;
            }

            auto colon_pos = line.find(':');
            if (colon_pos == std::string::npos) continue;
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            if (key == "player_name") data.set_player_name(value);
            else if (key == "level") data.set_level(std::stoi(value));
            else if (key == "health") data.set_health(std::stof(value));
            else if (key == "position") {
                auto c1 = value.find(',');
                auto c2 = value.rfind(',');
                if (c1 != std::string::npos && c2 != std::string::npos && c1 != c2) {
                    data.set_position(
                        std::stof(value.substr(0, c1)),
                        std::stof(value.substr(c1 + 1, c2 - c1 - 1)),
                        std::stof(value.substr(c2 + 1))
                    );
                }
            }
            else if (key == "flags") data.set_flags(std::stoull(value));
            else if (key == "version") data.set_version(std::stoi(value));
        }
        data.set_inventory(inventory);
        for (const auto& q : quests) data.set_quest_progress(q.first, q.second);
        file.close();
        return save_profile(profile_name, data);
    } catch (...) {
        return false;
    }
}

} // namespace arenax3
} // namespace com