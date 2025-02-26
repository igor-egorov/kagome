/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include "api/transport/impl/http/http_listener_impl.hpp"

#include <boost/asio.hpp>
#include <utility>

namespace kagome::api {
  HttpListenerImpl::HttpListenerImpl(
      const std::shared_ptr<application::AppStateManager> &app_state_manager,
      std::shared_ptr<Context> context,
      Configuration listener_config,
      SessionImpl::Configuration session_config)
      : context_{std::move(context)},
        config_{std::move(listener_config)},
        session_config_{session_config},
        logger_{log::createLogger("RpcHttpListener", "rpc_transport")} {
    BOOST_ASSERT(app_state_manager);
    app_state_manager->takeControl(*this);
  }

  bool HttpListenerImpl::prepare() {
    try {
      acceptor_ = std::make_unique<Acceptor>(*context_, config_.endpoint);
    } catch (const boost::wrapexcept<boost::system::system_error> &exception) {
      logger_->critical("Failed to prepare a listener: {}", exception.what());
      return false;
    } catch (const std::exception &exception) {
      logger_->critical("Exception when preparing a listener: {}",
                        exception.what());
      return false;
    }

    boost::system::error_code ec;
    acceptor_->set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
      logger_->error("Failed to set `reuse address` option to acceptor");
      return false;
    }
    return true;
  }

  bool HttpListenerImpl::start() {
    BOOST_ASSERT(acceptor_);

    if (!acceptor_->is_open()) {
      logger_->error("error: trying to start on non opened acceptor");
      return false;
    }

    acceptOnce();
    return true;
  }

  void HttpListenerImpl::stop() {
    if (acceptor_) {
      acceptor_->cancel();
    }
  }

  void HttpListenerImpl::setHandlerForNewSession(
      NewSessionHandler &&on_new_session) {
    on_new_session_ =
        std::make_unique<NewSessionHandler>(std::move(on_new_session));
  }

  void HttpListenerImpl::acceptOnce() {
    new_session_ = std::make_shared<SessionImpl>(*context_, session_config_);

    auto on_accept = [wp = weak_from_this()](boost::system::error_code ec) {
      if (auto self = wp.lock()) {
        if (not ec) {
          if (self->on_new_session_) {
            (*self->on_new_session_)(self->new_session_);
          }
          self->new_session_->start();
        }

        if (self->acceptor_->is_open()) {
          // continue to accept until acceptor is ready
          self->acceptOnce();
        }
      }
    };

    acceptor_->async_accept(new_session_->socket(), std::move(on_accept));
  }

}  // namespace kagome::api
