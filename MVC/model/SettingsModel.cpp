#include "SettingsModel.h"
#include <fstream>
#include <string>

bool SettingsModel::saveToFile(const std::string& path) const {
    std::ofstream f(path);
    if (!f.is_open()) return false;
    f << "destination="       << m_settings.destination                    << "\n";
    f << "backupName="        << m_settings.backupName                     << "\n";
    f << "autoBackup="        << (m_settings.autoBackup        ? 1 : 0)   << "\n";
    f << "interval="          << m_settings.interval                       << "\n";
    f << "maxCopies="         << m_settings.maxCopies                      << "\n";
    f << "includeSubfolders=" << (m_settings.includeSubfolders ? 1 : 0)   << "\n";
    f << "includeHidden="     << (m_settings.includeHidden     ? 1 : 0)   << "\n";
    f << "showNotifications=" << (m_settings.showNotifications ? 1 : 0)   << "\n";
    f << "strategy="          << (int)m_settings.strategy                  << "\n";
    f << "retryOnFailure="    << (m_settings.retryOnFailure    ? 1 : 0)   << "\n";
    f << "backupOnAppStart="  << (m_settings.backupOnAppStart  ? 1 : 0)   << "\n";
    return f.good();
}

bool SettingsModel::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if      (key == "destination")       m_settings.destination       = val;
        else if (key == "backupName")        m_settings.backupName        = val;
        else if (key == "autoBackup")        m_settings.autoBackup        = (val == "1");
        else if (key == "interval")          { try { m_settings.interval        = std::stoi(val); } catch(...){} }
        else if (key == "maxCopies")         { try { m_settings.maxCopies       = std::stoi(val); } catch(...){} }
        else if (key == "includeSubfolders") m_settings.includeSubfolders = (val == "1");
        else if (key == "includeHidden")     m_settings.includeHidden     = (val == "1");
        else if (key == "showNotifications") m_settings.showNotifications = (val == "1");
        else if (key == "strategy")          { try { m_settings.strategy = (BackupStrategy)std::stoi(val); } catch(...){} }
        else if (key == "retryOnFailure")    m_settings.retryOnFailure    = (val == "1");
        else if (key == "backupOnAppStart")  m_settings.backupOnAppStart  = (val == "1");
    }
    for (auto& cb : m_observers) cb();
    return true;
}
