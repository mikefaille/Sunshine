/**
 * @file tests/unit/platform/linux/test_audio.cpp
 * @brief Tests for Linux audio sink handling.
 */

#ifdef __linux__

  // test includes
  #include "../../../tests_common.h"

  // local includes
  #include "src/config.h"

namespace {
  /**
   * @brief Restore the keep_default_sink setting when a test scope ends.
   */
  class keep_default_sink_guard_t {
  public:
    /**
     * @brief Save the current keep_default_sink value.
     */
    keep_default_sink_guard_t():
        original_ {config::audio.keep_default_sink} {
    }

    /**
     * @brief Restore the saved keep_default_sink value.
     */
    ~keep_default_sink_guard_t() {
      config::audio.keep_default_sink = original_;
    }

  private:
    bool original_;  ///< Value present before the test changed the setting.
  };
}  // namespace

TEST(KeepDefaultSinkAudioTest, CaptureSinkKeepsHostDefaultSink) {
  keep_default_sink_guard_t guard;
  config::audio.keep_default_sink = true;

  auto control = platf::audio_control();
  if (!control) {
    GTEST_SKIP() << "PulseAudio is unavailable in this environment";
  }

  ASSERT_TRUE(control->supports_keep_default_sink());

  auto before = control->sink_info();
  ASSERT_TRUE(before.has_value());

  // A nonexistent sink proves the capture path never swaps: set_sink would fail with -1.
  EXPECT_EQ(control->capture_sink("sunshine-test-nonexistent-sink"), 0);

  auto after = control->sink_info();
  ASSERT_TRUE(after.has_value());
  EXPECT_EQ(after->host, before->host);
}

TEST(NullSinkOwnershipTest, AdoptedSinksSurvivePeerTeardown) {
  auto first = platf::audio_control();
  if (!first) {
    GTEST_SKIP() << "PulseAudio is unavailable in this environment";
  }

  auto first_sink = first->sink_info();
  ASSERT_TRUE(first_sink.has_value());
  ASSERT_TRUE(first_sink->null.has_value()) << "Sunshine null sinks missing from the graph";

  // A second session adopts the same sinks; tearing it down must not unload
  // sinks it didn't create (previously this yanked routed streams back to
  // the host default on every disconnect).
  {
    auto second = platf::audio_control();
    ASSERT_TRUE(second) << "PulseAudio rejected a second control connection";
    auto second_sink = second->sink_info();
    ASSERT_TRUE(second_sink.has_value());
    ASSERT_TRUE(second_sink->null.has_value());
  }

  auto after = first->sink_info();
  ASSERT_TRUE(after.has_value());
  EXPECT_TRUE(after->null.has_value()) << "Peer teardown unloaded adopted null sinks";
}

#endif  // __linux__
