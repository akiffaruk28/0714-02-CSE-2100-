#include "settings.h"
#include "fileops.h"
#include <fstream>
#include <sstream>

// SRP: SettingsManager only reads/writes BackupConfig to disk.
//      It does NOT start timers, create directories, or touch GTK.
// DIP: Implements ISettingsRepository — callers depend on the interface.

SettingsManager::SettingsManager(const std::string& configFilePath)
    : m_configFilePath(configFilePath)
{}

bool SettingsManager::load() {
    std::ifstream file(m_configFilePath);
    if (!file.is_open()) {
        save();   // write defaults on first run
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto sep = line.find('=');
        if (sep == std::string::npos) continue;

        std::string key   = line.substr(0, sep);
        std::string value = line.substr(sep + 1);

        if      (key == "destination") m_config.backupDestination = value;
        else if (key == "auto_backup") m_config.autoBackup        = (std::stoi(value) != 0);
        else if (key == "interval")    m_config.backupInterval    = std::stoi(value);
        else if (key == "max_copies")  m_config.maxCopies         = std::stoi(value);
        else if (key == "subfolders")  m_config.backupSubfolders  = (std::stoi(value) != 0);
        else if (key == "hidden")      m_config.includeHidden     = (std::stoi(value) != 0);
        else if (key == "notify")      m_config.showNotifications = (std::stoi(value) != 0);
    }

    // Ensure the destination directory exists
    FileOperations::createDirectory(m_config.backupDestination);
    return true;
}

bool SettingsManager::save() {
    std::ofstream file(m_configFilePath);
    if (!file.is_open()) return false;

    file << "destination=" << m_config.backupDestination << "\n";
    file << "auto_backup=" << m_config.autoBackup        << "\n";
    file << "interval="    << m_config.backupInterval    << "\n";
    file << "max_copies="  << m_config.maxCopies         << "\n";
    file << "subfolders="  << m_config.backupSubfolders  << "\n";
    file << "hidden="      << m_config.includeHidden     << "\n";
    file << "notify="      << m_config.showNotifications << "\n";
    return true;
}

BackupConfig& SettingsManager::getConfig() {
    return m_config;
}

const BackupConfig& SettingsManager::getConfig() const {
    return m_config;
}

void SettingsManager::setConfig(const BackupConfig& config) {
    m_config = config;
}
