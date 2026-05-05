#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
import gpiod
import time
import threading

CHIP = "gpiochip4"
GPIO_PIN = 8          # Rubik Pi 40-pin header GPIO7
PULSE_US = 50         # 50 microsecond pulse (match Arduino)
INTERVAL_S = 0.01     # 10ms = 100Hz

class TriggerNode(Node):
    def __init__(self):
        super().__init__('trigger_node')
        try:
            self.chip = gpiod.Chip(CHIP)
            self.line = self.chip.get_line(GPIO_PIN)
            self.line.request(
                consumer="trigger_node",
                type=gpiod.LINE_REQ_DIR_OUT,
                default_val=0
            )
        except Exception as e:
            print(f"GPIO error: {e}")
            raise

        self.running = True
        self.thread = threading.Thread(target=self.trigger_loop, daemon=True)
        self.thread.start()
        self.get_logger().info(f"Trigger node started on {CHIP} pin {GPIO_PIN} at 100Hz")

    def trigger_loop(self):
        while self.running:
            loop_start = time.monotonic()

            # Pulse HIGH for 50µs
            self.line.set_value(1)
            time.sleep(PULSE_US / 1_000_000)
            self.line.set_value(0)

            # Sleep for remainder of 10ms interval
            elapsed = time.monotonic() - loop_start
            remaining = INTERVAL_S - elapsed
            if remaining > 0:
                time.sleep(remaining)

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