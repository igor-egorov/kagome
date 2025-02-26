/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_BABE_SYNCHRONIZER_IMPL_HPP
#define KAGOME_BABE_SYNCHRONIZER_IMPL_HPP

#include "consensus/babe/babe_synchronizer.hpp"

#include "application/app_configuration.hpp"
#include "log/logger.hpp"
#include "network/types/sync_clients_set.hpp"

namespace kagome::consensus {

  /**
   * Implementation of babe synchronizer that requests blocks from provided
   * peers
   */
  class BabeSynchronizerImpl
      : public BabeSynchronizer,
        public std::enable_shared_from_this<BabeSynchronizerImpl> {
   public:
    ~BabeSynchronizerImpl() override = default;

    BabeSynchronizerImpl(
        std::shared_ptr<network::SyncClientsSet> sync_clients,
        const application::AppConfiguration &app_configuration);

    void request(const primitives::BlockId &from,
                 const primitives::BlockHash &to,
                 const libp2p::peer::PeerId &peer_id,
                 const BlocksHandler &block_list_handler) override;

   private:
    /**
     * Select next client to be polled
     * @param polled_clients clients that we already polled
     * @return next clint to be polled
     */
    std::shared_ptr<network::SyncProtocolClient> selectNextClient(
        std::unordered_set<std::shared_ptr<network::SyncProtocolClient>>
            &polled_clients) const;
    /**
     * Request blocks from provided peers
     * @param request block request message
     * @param polled_clients peers that were already requested
     * @param requested_blocks_handler handler of received blocks
     */
    void pollClients(network::BlocksRequest request,
                     const libp2p::peer::PeerId &peer_id,
                     const BlocksHandler &requested_blocks_handler) const;

    std::shared_ptr<network::SyncClientsSet> sync_clients_;
    log::Logger logger_;
    const application::AppConfiguration &app_configuration_;
  };
}  // namespace kagome::consensus

#endif  // KAGOME_BABE_SYNCHRONIZER_IMPL_HPP
