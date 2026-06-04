// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#include "UidStore.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

namespace {

std::optional<uint16_t> parse_uid_decimal(const std::string& s) {
    if (s.empty()) return std::nullopt;
    char* end = nullptr;
    errno = 0;
    long v = std::strtol(s.c_str(), &end, 10);
    if (errno != 0 || end == s.c_str() || v < 0 || v > 0xFFFF) {
        return std::nullopt;
    }
    // Skip trailing whitespace, accept "12345\n" etc.
    while (end && *end != '\0') {
        if (*end != '\n' && *end != '\r' && *end != ' ' && *end != '\t') {
            return std::nullopt;
        }
        ++end;
    }
    return static_cast<uint16_t>(v);
}

} // namespace

FileUidStore::FileUidStore(std::string path) : m_path(std::move(path)) {}

std::optional<uint16_t> FileUidStore::load() {
    std::ifstream f(m_path);
    if (!f) return std::nullopt;

    std::string contents;
    {
        std::string line;
        std::getline(f, line);
        contents = std::move(line);
    }

    auto parsed = parse_uid_decimal(contents);
    if (!parsed) {
        std::cerr << "FileUidStore: malformed contents in '" << m_path
                  << "', ignoring." << std::endl;
        return std::nullopt;
    }
    return parsed;
}

void FileUidStore::save(uint16_t uid) {
    namespace fs = std::filesystem;
    fs::path target(m_path);
    fs::path parent = target.parent_path();
    if (!parent.empty()) {
        std::error_code ec;
        fs::create_directories(parent, ec);
        if (ec) {
            std::cerr << "FileUidStore: failed to create dir '"
                      << parent.string() << "': " << ec.message() << std::endl;
            return;
        }
    }

    fs::path tmp = target;
    tmp += ".tmp";

    {
        std::ofstream f(tmp, std::ios::trunc);
        if (!f) {
            std::cerr << "FileUidStore: failed to open '" << tmp.string()
                      << "' for write." << std::endl;
            return;
        }
        f << uid << '\n';
        if (!f) {
            std::cerr << "FileUidStore: write to '" << tmp.string()
                      << "' failed." << std::endl;
            return;
        }
    }

    std::error_code ec;
    fs::rename(tmp, target, ec);
    if (ec) {
        std::cerr << "FileUidStore: rename '" << tmp.string() << "' -> '"
                  << target.string() << "' failed: " << ec.message() << std::endl;
        std::error_code ec2;
        fs::remove(tmp, ec2);
    }
}

void FileUidStore::clear() {
    std::error_code ec;
    std::filesystem::remove(m_path, ec);
    if (ec && ec != std::errc::no_such_file_or_directory) {
        std::cerr << "FileUidStore: clear failed for '" << m_path
                  << "': " << ec.message() << std::endl;
    }
}

EnvOverrideUidStore::EnvOverrideUidStore(std::string env_var,
                                         std::unique_ptr<IUidStore> backing)
    : m_env_var(std::move(env_var)), m_backing(std::move(backing)) {}

std::optional<uint16_t> EnvOverrideUidStore::load() {
    const char* v = std::getenv(m_env_var.c_str());
    if (v && *v) {
        auto parsed = parse_uid_decimal(v);
        if (parsed) return parsed;
        std::cerr << "EnvOverrideUidStore: " << m_env_var << "='" << v
                  << "' is not a valid UID, ignoring." << std::endl;
    }
    return m_backing ? m_backing->load() : std::nullopt;
}

void EnvOverrideUidStore::save(uint16_t uid) {
    const char* v = std::getenv(m_env_var.c_str());
    if (v && *v) {
        // Env override active: do not persist, so the override stays
        // authoritative across runs without leaving on-disk residue.
        return;
    }
    if (m_backing) m_backing->save(uid);
}

void EnvOverrideUidStore::clear() {
    if (m_backing) m_backing->clear();
}
