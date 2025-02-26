/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <thread>

#include "application/impl/app_state_manager_impl.hpp"
#include "testutil/prepare_loggers.hpp"

using kagome::application::AppStateException;
using kagome::application::AppStateManager;
using kagome::application::AppStateManagerImpl;
using OnPrepare = kagome::application::AppStateManager::OnPrepare;
using OnLaunch = kagome::application::AppStateManager::OnLaunch;
using OnShutdown = kagome::application::AppStateManager::OnShutdown;

using testing::Return;
using testing::Sequence;

class OnPrepareMock {
 public:
  MOCK_METHOD0(call, bool());
  bool operator()() {
    return call();
  }
};
class OnLaunchMock {
 public:
  MOCK_METHOD0(call, bool());
  bool operator()() {
    return call();
  }
};
class OnShutdownMock {
 public:
  MOCK_METHOD0(call, void());
  void operator()() {
    return call();
  }
};

class AppStateManagerTest : public AppStateManagerImpl, public testing::Test {
 public:
  static void SetUpTestCase() {
    testutil::prepareLoggers();
  }

  void SetUp() override {
    reset();
    prepare_cb = std::make_shared<OnPrepareMock>();
    launch_cb = std::make_shared<OnLaunchMock>();
    shutdown_cb = std::make_shared<OnShutdownMock>();
  }
  void TearDown() override {
    prepare_cb.reset();
    launch_cb.reset();
    shutdown_cb.reset();
  }

  std::shared_ptr<OnPrepareMock> prepare_cb;
  std::shared_ptr<OnLaunchMock> launch_cb;
  std::shared_ptr<OnShutdownMock> shutdown_cb;
};

/**
 * @given new created AppStateManager
 * @when switch stages in order
 * @then state changes according to the order
 */
TEST_F(AppStateManagerTest, StateSequence_Normal) {
  ASSERT_EQ(state(), AppStateManager::State::Init);
  ASSERT_NO_THROW(doPrepare());
  ASSERT_EQ(state(), AppStateManager::State::ReadyToStart);
  ASSERT_NO_THROW(doLaunch());
  ASSERT_EQ(state(), AppStateManager::State::Works);
  ASSERT_NO_THROW(doShutdown());
  ASSERT_EQ(state(), AppStateManager::State::ReadyToStop);
}

/**
 * @given AppStateManager in state 'ReadyToStart' (after stage 'prepare')
 * @when trying to run stage 'prepare' again
 * @then thrown exception, state wasn't change. Can to run stages 'launch' and
 * 'shutdown'
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_1) {
  doPrepare();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_EQ(state(), AppStateManager::State::ReadyToStart);
  EXPECT_NO_THROW(doLaunch());
  EXPECT_NO_THROW(doShutdown());
}

/**
 * @given AppStateManager in state 'Works' (after stage 'launch')
 * @when trying to run stages 'prepare' and 'launch' again
 * @then thrown exceptions, state wasn't change. Can to run stages 'launch' and
 * 'shutdown'
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_2) {
  doPrepare();
  doLaunch();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_THROW(doLaunch(), AppStateException);
  EXPECT_EQ(state(), AppStateManager::State::Works);
  EXPECT_NO_THROW(doShutdown());
}

/**
 * @given AppStateManager in state 'ReadyToStop' (after stage 'shutdown')
 * @when trying to run any stages 'prepare' and 'launch' again
 * @then thrown exceptions, except shutdown. State wasn't change.
 */
TEST_F(AppStateManagerTest, StateSequence_Abnormal_3) {
  doPrepare();
  doLaunch();
  doShutdown();
  EXPECT_THROW(doPrepare(), AppStateException);
  EXPECT_THROW(doLaunch(), AppStateException);
  EXPECT_NO_THROW(doShutdown());
  EXPECT_EQ(state(), AppStateManager::State::ReadyToStop);
}

/**
 * @given new created AppStateManager
 * @when add callbacks for each stages
 * @then done without exceptions
 */
TEST_F(AppStateManagerTest, AddCallback_Initial) {
  EXPECT_NO_THROW(atPrepare(OnPrepare{}));
  EXPECT_NO_THROW(atLaunch(OnLaunch{}));
  EXPECT_NO_THROW(atShutdown(OnShutdown{}));
}

/**
 * @given AppStateManager in state 'ReadyToStart' (after stage 'prepare')
 * @when add callbacks for each stages
 * @then thrown exception only for 'prepare' stage callback
 */
TEST_F(AppStateManagerTest, AddCallback_AfterPrepare) {
  doPrepare();
  EXPECT_THROW(atPrepare(OnPrepare{}), AppStateException);
  EXPECT_NO_THROW(atLaunch(OnLaunch{}));
  EXPECT_NO_THROW(atShutdown(OnShutdown{}));
}

/**
 * @given AppStateManager in state 'Works' (after stage 'launch')
 * @when add callbacks for each stages
 * @then done without exception only for 'shutdown' stage callback
 */
TEST_F(AppStateManagerTest, AddCallback_AfterLaunch) {
  doPrepare();
  doLaunch();
  EXPECT_THROW(atPrepare(OnPrepare{}), AppStateException);
  EXPECT_THROW(atLaunch(OnLaunch{}), AppStateException);
  EXPECT_NO_THROW(atShutdown(OnShutdown{}));
}

/**
 * @given AppStateManager in state 'ReadyToStop' (after stage 'shutdown')
 * @when add callbacks for each stages
 * @then trown exceptions for each calls
 */
TEST_F(AppStateManagerTest, AddCallback_AfterShutdown) {
  doPrepare();
  doLaunch();
  doShutdown();
  EXPECT_THROW(atPrepare(OnPrepare{}), AppStateException);
  EXPECT_THROW(atLaunch(OnLaunch{}), AppStateException);
  EXPECT_THROW(atShutdown(OnShutdown{}), AppStateException);
}

/**
 * @given new created AppStateManager
 * @when register callbacks by reg() method
 * @then each callcack registered for appropriate state
 */
TEST_F(AppStateManagerTest, RegCallbacks) {
  int tag = 0;

  registerHandlers(
      [&] {
        tag = 1;
        return (*prepare_cb)();
      },
      [&] {
        tag = 2;
        return (*launch_cb)();
      },
      [&] {
        tag = 3;
        return (*shutdown_cb)();
      });

  EXPECT_CALL(*prepare_cb, call()).WillOnce(Return(true));
  EXPECT_CALL(*launch_cb, call()).WillOnce(Return(true));
  EXPECT_CALL(*shutdown_cb, call()).WillOnce(Return());

  EXPECT_NO_THROW(doPrepare());
  EXPECT_EQ(tag, 1);
  EXPECT_NO_THROW(doLaunch());
  EXPECT_EQ(tag, 2);
  EXPECT_NO_THROW(doShutdown());
  EXPECT_EQ(tag, 3);
}

/**
 * @given new created AppStateManager
 * @when register callbacks by reg() method and run() AppStateManager
 * @then each callcack execudted according to the stages order
 */
TEST_F(AppStateManagerTest, Run_CallSequence) {
  EXPECT_THROW(run(), std::logic_error);

  auto app_state_manager = std::make_shared<AppStateManagerImpl>();

  app_state_manager->registerHandlers([&] { return (*prepare_cb)(); },
                                      [&] { return (*launch_cb)(); },
                                      [&] { return (*shutdown_cb)(); });

  Sequence seq;
  EXPECT_CALL(*prepare_cb, call()).InSequence(seq).WillOnce(Return(true));
  EXPECT_CALL(*launch_cb, call()).InSequence(seq).WillOnce(Return(true));
  EXPECT_CALL(*shutdown_cb, call()).InSequence(seq).WillOnce(Return());

  app_state_manager->atLaunch([] {
    std::thread terminator([] { raise(SIGQUIT); });
    terminator.detach();
    return true;
  });

  EXPECT_NO_THROW(app_state_manager->run());
}
