/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL
#define KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL

#include "storage/trie/trie_storage.hpp"

#include "log/logger.hpp"
#include "primitives/event_types.hpp"
#include "storage/changes_trie/changes_tracker.hpp"
#include "storage/trie/codec.hpp"
#include "storage/trie/polkadot_trie/polkadot_trie_factory.hpp"
#include "storage/trie/serialization/trie_serializer.hpp"

namespace kagome::storage::trie {

  class TrieStorageImpl : public TrieStorage {
   public:
    static outcome::result<std::unique_ptr<TrieStorageImpl>> createEmpty(
        const std::shared_ptr<PolkadotTrieFactory> &trie_factory,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

    static outcome::result<std::unique_ptr<TrieStorageImpl>> createFromStorage(
        const RootHash &root_hash,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

    TrieStorageImpl(TrieStorageImpl const &) = delete;
    void operator=(const TrieStorageImpl &) = delete;

    TrieStorageImpl(TrieStorageImpl &&) = default;
    TrieStorageImpl &operator=(TrieStorageImpl &&) = default;
    ~TrieStorageImpl() override = default;

    outcome::result<std::unique_ptr<PersistentTrieBatch>> getPersistentBatch()
        override;
    outcome::result<std::unique_ptr<EphemeralTrieBatch>> getEphemeralBatch()
        const override;

    outcome::result<std::unique_ptr<PersistentTrieBatch>> getPersistentBatchAt(
        const RootHash &root) override;
    outcome::result<std::unique_ptr<EphemeralTrieBatch>> getEphemeralBatchAt(
        const RootHash &root) const override;

    RootHash getRootHash() const noexcept override;

   protected:
    TrieStorageImpl(
        RootHash root_hash,
        std::shared_ptr<Codec> codec,
        std::shared_ptr<TrieSerializer> serializer,
        boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes);

   private:
    RootHash root_hash_;
    std::shared_ptr<Codec> codec_;
    std::shared_ptr<TrieSerializer> serializer_;
    boost::optional<std::shared_ptr<changes_trie::ChangesTracker>> changes_;
    log::Logger logger_;
  };

}  // namespace kagome::storage::trie

#endif  // KAGOME_STORAGE_TRIE_IMPL_TRIE_STORAGE_IMPL
