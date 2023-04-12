#!/usr/bin/env python3
import argparse
import time
import multiprocessing

import paho.mqtt.client as mqtt


def consumer(barrier, conn, host, expected_messages, timeout):
    pid = multiprocessing.current_process().pid
    client = mqtt.Client(f"consumer_{pid}")
    # client.username_pw_set("username", "password")

    total_messages = 0
    first_message_time = None
    last_message_time = None

    def on_message(client, userdata, msg):
        nonlocal total_messages, first_message_time, last_message_time
        total_messages += 1
        last_message_time = time.time()
        first_message_time = first_message_time or last_message_time

    def on_subscribe(client, userdata, mid, granted_qos):
        barrier.wait()

    client.on_message = on_message
    client.on_subscribe = on_subscribe

    client.connect(host)
    client.loop_start()
    client.subscribe("benchmark")
    while total_messages < expected_messages:
        if first_message_time and total_messages >= 2:
            elapsed_time = time.time() - first_message_time
            if elapsed_time >= timeout:
                break
        time.sleep(1)
    client.loop_stop()

    elapsed_time = last_message_time - first_message_time
    rate = (total_messages - 1) / elapsed_time
    conn.send(rate)


parser = argparse.ArgumentParser()
parser.add_argument("host")
parser.add_argument("--consumers", type=int, default=1)
parser.add_argument("--messages", type=int, default=1000)
parser.add_argument("--size", type=int, default=1)
parser.add_argument("--timeout", type=int, default=10)

args = parser.parse_args()

client = mqtt.Client("producer")
client.connect(args.host)

barrier = multiprocessing.Barrier(args.consumers + 1)
pipe = multiprocessing.Pipe(False)
consumers = [
    multiprocessing.Process(
        target=consumer, args=(barrier, pipe[1], args.host, args.messages, args.timeout)
    )
    for _ in range(args.consumers)
]

message = "0" * args.size

for consumer in consumers:
    consumer.start()

# wait for all processes to connect
barrier.wait()

# fire messages
while any(p.is_alive() for p in consumers):
    client.publish("benchmark", message)

# wait for consumers
for consumer in consumers:
    consumer.join()

# collect results
rate = sum(pipe[0].recv() for _ in consumers) / len(consumers)
print(f"{rate:.1f}")
