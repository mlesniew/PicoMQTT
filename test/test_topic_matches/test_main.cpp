#include <Arduino.h>
#include <unity.h>

#include "PicoMQTT/subscriber.h"

using PicoMQTT::Subscriber;

void test_exact_match() {
    TEST_ASSERT_TRUE(Subscriber::topic_matches("home/livingroom/temp",
                                               "home/livingroom/temp"));
    TEST_ASSERT_FALSE(Subscriber::topic_matches("home/livingroom/temp",
                                                "home/livingroom/humidity"));
}

void test_single_level_wildcard_plus() {
    TEST_ASSERT_TRUE(
        Subscriber::topic_matches("home/+/temp", "home/kitchen/temp"));
    TEST_ASSERT_FALSE(
        Subscriber::topic_matches("home/+/temp", "home/kitchen/inside/temp"));
}

void test_multi_level_wildcard_hash_tail() {
    TEST_ASSERT_TRUE(Subscriber::topic_matches("home/#", "home/kitchen/temp"));
    TEST_ASSERT_TRUE(Subscriber::topic_matches("#", "home/kitchen/temp"));
}

void test_hash_should_match_parent_topic_too() {
    TEST_ASSERT_TRUE(Subscriber::topic_matches("home/#", "home"));
    TEST_ASSERT_TRUE(Subscriber::topic_matches("home/#", "home/"));
}

void test_non_matching_prefix() {
    TEST_ASSERT_FALSE(
        Subscriber::topic_matches("office/#", "home/kitchen/temp"));
}

void test_plus_does_not_match_missing_level() {
    TEST_ASSERT_FALSE(Subscriber::topic_matches("home/+/temp", "home/temp"));
}

void setup() {
    UNITY_BEGIN();

    RUN_TEST(test_exact_match);
    RUN_TEST(test_single_level_wildcard_plus);
    RUN_TEST(test_multi_level_wildcard_hash_tail);
    RUN_TEST(test_hash_should_match_parent_topic_too);
    RUN_TEST(test_non_matching_prefix);
    RUN_TEST(test_plus_does_not_match_missing_level);

    UNITY_END();
}

void loop() {}
