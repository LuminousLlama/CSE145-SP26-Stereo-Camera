#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
import gpiod
import time
import threading
import os

CHIP = "gpiochip4"
GPIO_PIN = 8

PULSE_US = 50
INTERVAL_S = 0.01   # 100 Hz

class TriggerNode(Node):
    def __init__(self):
        super().__init__('trigger_node')

        # Try to improve scheduling priority (best effort)
        try:
            os.sched_setscheduler(0, os.SCHED_FIFO, os.sched_param(80))
            self.get_logger().info("Set real-time scheduler (SCHED_FIFO)")
        except PermissionError:
            self.get_logger().warn("Could not set real-time scheduler (run as sudo?)")

        # Setup GPIO
        self.chip = gpiod.Chip(CHIP)
        self.line = self.chip.get_line(GPIO_PIN)
        self.line.request(
            consumer="trigger_node",
            type=gpiod.LINE_REQ_DIR_OUT,
            default_val=0
        )

        self.running = True
        self.thread = threading.Thread(target=self.trigger_loop, daemon=True)
        self.thread.start()

        self.get_logger().info(
            f"Trigger running on {CHIP}:{GPIO_PIN} @ {1/INTERVAL_S:.1f} Hz, pulse {PULSE_US} us"
        )

    def busy_wait_us(self, duration_us):
        start = time.perf_counter()
        target = duration_us / 1_000_000.0
        while (time.perf_counter() - start) < target:
            pass

    def trigger_loop(self):
        next_time = time.perf_counter()

        while self.running:
            # Wait until next scheduled time
            now = time.perf_counter()
            if now < next_time:
                time.sleep(next_time - now)

            # FIRE TRIGGER
            self.line.set_value(1)
            self.busy_wait_us(PULSE_US)
            self.line.set_value(0)

            # Schedule next cycle (avoids drift)
            next_time += INTERVAL_S

    def destroy_node(self):
        self.running = False
        self.thread.join()
        self.line.set_value(0)
        self.line.release()
        self.chip.close()
        super().destroy_node()

def main():
    rclpy.init()
    node = TriggerNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == "__main__":
    main()