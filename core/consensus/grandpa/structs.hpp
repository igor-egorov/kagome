/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef KAGOME_CONSENSUS_GRANDPA_STRUCTS_
#define KAGOME_CONSENSUS_GRANDPA_STRUCTS_

#include <boost/asio/steady_timer.hpp>
#include <boost/variant.hpp>
#include <common/buffer.hpp>

#include "common/blob.hpp"
#include "common/buffer.hpp"
#include "common/visitor.hpp"
#include "common/wrapper.hpp"
#include "consensus/grandpa/common.hpp"
#include "crypto/ed25519_types.hpp"
#include "log/logger.hpp"
#include "primitives/authority.hpp"
#include "primitives/common.hpp"
#include "scale/scale.hpp"

namespace kagome::consensus::grandpa {

  using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

  using BlockInfo = primitives::BlockInfo;
  using Precommit = primitives::detail::BlockInfoT<struct PrecommitTag>;
  using Prevote = primitives::detail::BlockInfoT<struct PrevoteTag>;
  using PrimaryPropose =
      primitives::detail::BlockInfoT<struct PrimaryProposeTag>;

  /// Note: order of types in variant matters
  using Vote = boost::variant<Prevote,          // 0
                              Precommit,        // 1
                              PrimaryPropose>;  // 2

  struct SignedMessage {
    Vote message;
    Signature signature;
    Id id;

    BlockNumber block_number() const {
      return visit_in_place(message,
                            [](const auto &vote) { return vote.block_number; });
    }

    BlockHash block_hash() const {
      return visit_in_place(message,
                            [](const auto &vote) { return vote.block_hash; });
    }

    BlockInfo block_info() const {
      return visit_in_place(message, [](const auto &vote) {
        return BlockInfo{vote.block_number, vote.block_hash};
      });
    }

    template <typename T>
    bool is() const {
      return message.type() == typeid(T);
    }

    bool operator==(const SignedMessage &rhs) const {
      return message == rhs.message && signature == rhs.signature && id == id;
    }

    bool operator!=(const SignedMessage &rhs) const {
      return !operator==(rhs);
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const SignedMessage &signed_msg) {
    return s << (scale::encode(signed_msg.message).value())
             << signed_msg.signature << signed_msg.id;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, SignedMessage &signed_msg) {
    common::Buffer encoded_vote;
    s >> encoded_vote;
    auto decoded_vote = scale::template decode<Vote>(encoded_vote).value();
    signed_msg.message = decoded_vote;
    return s >> signed_msg.signature >> signed_msg.id;
  }

  template <typename Message>
  struct Equivocated {
    Message first;
    Message second;
  };

  using VotingMessage = SignedMessage;
  using EquivocatoryVotingMessage = std::pair<VotingMessage, VotingMessage>;
  using VoteVariant = boost::variant<VotingMessage, EquivocatoryVotingMessage>;

  namespace detail {
    /// Proof of an equivocation (double-vote) in a given round.
    template <typename Message>
    struct Equivocation {  // NOLINT
      /// The round number equivocated in.
      RoundNumber round;
      /// The identity of the equivocator.
      Id id;
      Equivocated<Message> proof;
    };
  }  // namespace detail

  class SignedPrecommit : public SignedMessage {
    using SignedMessage::SignedMessage;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const SignedPrecommit &signed_msg) {
    assert(signed_msg.template is<Precommit>());
    return s << boost::strict_get<Precommit>(signed_msg.message)
             << signed_msg.signature << signed_msg.id;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, SignedPrecommit &signed_msg) {
    signed_msg.message = Precommit{};
    return s >> boost::strict_get<Precommit>(signed_msg.message)
           >> signed_msg.signature >> signed_msg.id;
  }

  // justification that contains a list of signed precommits justifying the
  // validity of the block
  struct GrandpaJustification {
    RoundNumber round_number;
    BlockInfo block_info;
    std::vector<SignedPrecommit> items;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const GrandpaJustification &v) {
    return s << v.round_number << v.block_info << v.items;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, GrandpaJustification &v) {
    return s >> v.round_number >> v.block_info >> v.items;
  }

  /// A commit message which is an aggregate of precommits.
  struct Commit {
    BlockInfo vote;
    GrandpaJustification justification;
  };

  // either prevote, precommit or primary propose
  struct VoteMessage {
    RoundNumber round_number{0};
    MembershipCounter counter{0};
    SignedMessage vote;

    Id id() const {
      return vote.id;
    }
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const VoteMessage &v) {
    return s << v.round_number << v.counter << v.vote;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, VoteMessage &v) {
    return s >> v.round_number >> v.counter >> v.vote;
  }

  // finalizing message
  struct Fin {
    RoundNumber round_number{0};
    BlockInfo vote;
    GrandpaJustification justification;
  };

  template <class Stream,
            typename = std::enable_if_t<Stream::is_encoder_stream>>
  Stream &operator<<(Stream &s, const Fin &f) {
    return s << f.round_number << f.vote << f.justification;
  }

  template <class Stream,
            typename = std::enable_if_t<Stream::is_decoder_stream>>
  Stream &operator>>(Stream &s, Fin &f) {
    return s >> f.round_number >> f.vote >> f.justification;
  }

  using PrevoteEquivocation = detail::Equivocation<Prevote>;
  using PrecommitEquivocation = detail::Equivocation<Precommit>;

  struct TotalWeight {
    uint64_t prevote;
    uint64_t precommit;
  };
}  // namespace kagome::consensus::grandpa

#endif  // KAGOME_CONSENSUS_GRANDPA_STRUCTS_
