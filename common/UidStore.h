// This file is part of the Open Audio Live System project, a live audio environment
// Copyright (c) 2026 - Mathis DELGADO
//
// This project is distributed under the Creative Commons CC-BY-NC-SA licence. https://creativecommons.org/licenses/by-nc-sa/4.0

#ifndef OPENAUDIONETWORK_UID_STORE_H
#define OPENAUDIONETWORK_UID_STORE_H

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

/**
 * @class IUidStore
 * @brief Backend-agnostic persistence for the autoconfigured UID.
 *
 * Three concrete backings live alongside this interface: FileUidStore
 * (atomic file on Linux / host dev), NullUidStore (test harness, opt-out),
 * and JsonFieldUidStore (a field inside an existing per-instance JSON
 * config — phase 2). Firmware backings (STM32 flash, Zephyr settings)
 * land in phase 3.
 *
 * Errors are logged, never thrown — UID persistence is best-effort.
 * load() returning nullopt is the universal "no persisted UID" signal,
 * whether that means absent file, parse error, or wrong type.
 */
struct IUidStore {
    virtual ~IUidStore() = default;

    virtual std::optional<uint16_t> load() = 0;
    virtual void save(uint16_t uid) = 0;
    virtual void clear() = 0;
};

/**
 * @class NullUidStore
 * @brief No-op persistence. Use in tests and in processes that opt out
 *        of UID persistence entirely. load() always returns nullopt,
 *        save()/clear() do nothing.
 */
class NullUidStore : public IUidStore {
public:
    std::optional<uint16_t> load() override { return std::nullopt; }
    void save(uint16_t /*uid*/) override {}
    void clear() override {}
};

/**
 * @class FileUidStore
 * @brief Stores the UID as a single decimal number in a plain text file.
 *
 * Atomic on save (write-temp-then-rename). The file's parent directory
 * is created on demand. Designed for Linux engine, Linux UI, and
 * host-backend dev builds. Each instance on a multi-process host should
 * point at a distinct path.
 */
class FileUidStore : public IUidStore {
public:
    explicit FileUidStore(std::string path);

    std::optional<uint16_t> load() override;
    void save(uint16_t uid) override;
    void clear() override;

private:
    std::string m_path;
};

/**
 * @class EnvOverrideUidStore
 * @brief One-shot pin via environment variable.
 *
 * Wraps a backing store. If the named env var is set and parses as a
 * valid UID, load() returns that value and save() is a no-op (so test
 * scripts can pin without polluting on-disk state). If the env var is
 * unset, all operations delegate to the backing store.
 *
 * Typical use: `OAN_PERSISTED_UID=12345 OALSEngine sim:default`.
 */
class EnvOverrideUidStore : public IUidStore {
public:
    EnvOverrideUidStore(std::string env_var, std::unique_ptr<IUidStore> backing);

    std::optional<uint16_t> load() override;
    void save(uint16_t uid) override;
    void clear() override;

private:
    std::string m_env_var;
    std::unique_ptr<IUidStore> m_backing;
};

#endif // OPENAUDIONETWORK_UID_STORE_H
