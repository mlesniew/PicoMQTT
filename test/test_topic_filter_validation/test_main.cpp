#include <Arduino.h>
#include <unity.h>

#include "PicoMQTT/subscriber.h"

using PicoMQTT::Subscriber;

void test_valid_filters() {
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("#"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("home/#"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("home/+/temp"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("+/status"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("home/+/+/state"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("home/"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("/home"));
    TEST_ASSERT_TRUE(Subscriber::is_valid_topic_filter("home//temp"));
}

void test_invalid_filters() {
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter(nullptr));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter(""));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home#"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home/#/temp"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home+"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home+room/temp"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home/+temp"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home/te+mp"));
    TEST_ASSERT_FALSE(Subscriber::is_valid_topic_filter("home/temp+"));
}

void setup() {
    UNITY_BEGIN();

    RUN_TEST(test_valid_filters);
    RUN_TEST(test_invalid_filters);

    UNITY_END();
}

void loop() {}
