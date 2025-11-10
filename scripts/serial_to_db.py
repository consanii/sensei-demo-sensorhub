#!/usr/bin/env python3

"""Stream CSV sensor readings from a serial port directly to InfluxDB."""

import argparse
import csv
import logging
import signal
import sys
import time
from datetime import datetime, timezone
from typing import Dict, List

import serial
from influxdb import InfluxDBClient


FIELD_ORDER: List[str] = [
    "Timestamp",
    "SCD41_CO2",
    "SCD41_Temperature",
    "SCD41_Humidity",
    "SGP41_VOC",
    "SGP41_NOX",
    "ILPS28QSW_Pressure",
    "ILPS28QSW_Temperature",
    "BME688_Temperature",
    "BME688_Pressure",
    "BME688_Humidity",
    "BME688_Gas_Resistance",
    "BH1730FVC_Visible",
    "BH1730FVC_IR",
    "BH1730FVC_Lux",
    "AS7331_Temperature",
    "AS7331_UVA",
    "AS7331_UVB",
    "AS7331_UVC",
]


def _graceful_shutdown(signum: int, frame) -> None:
    """Handle shutdown signals gracefully."""
    logging.info("Received signal %s, shutting down", signum)
    sys.exit(0)


def parse_csv_line(line: str) -> Dict[str, float]:
    """Convert a CSV line into a dict keyed by FIELD_ORDER."""
    reader = csv.reader([line], skipinitialspace=True)
    try:
        row = next(reader)
    except StopIteration as exc:
        raise ValueError("empty line") from exc

    if len(row) != len(FIELD_ORDER):
        raise ValueError(f"expected {len(FIELD_ORDER)} values, got {len(row)}")

    parsed: Dict[str, float] = {}
    for key, raw_value in zip(FIELD_ORDER, row):
        value = raw_value.strip()
        if not value:
            raise ValueError(f"missing value for {key}")
        if key == "Timestamp":
            parsed[key] = float(int(float(value)))
            continue
        parsed[key] = float(value)
    return parsed


def build_point(measurement: str, values: Dict[str, float]) -> dict:
    """Create an InfluxDB point dictionary from sensor values."""
    fields = {}
    
    # Add all fields except Timestamp
    for key, value in values.items():
        if key == "Timestamp":
            continue
        if value.is_integer():
            fields[key] = int(value)
        else:
            fields[key] = value
    
    return {
        "measurement": measurement,
        "time": datetime.now(timezone.utc),
        "fields": fields
    }


def run(args: argparse.Namespace) -> None:
    """Main processing loop: read serial data and write to InfluxDB."""
    # Initialize InfluxDB v1 client
    logging.info("Connecting to InfluxDB at %s:%s", args.influx_host, args.influx_port)
    client = InfluxDBClient(
        host=args.influx_host,
        port=args.influx_port,
        username=args.influx_user,
        password=args.influx_password,
        database=args.influx_database
    )
    
    logging.info("Opening serial port %s @ %d baud", args.serial_port, args.baudrate)
    
    try:
        # Open serial port
        try:
            ser = serial.Serial(
            port=args.serial_port,
            baudrate=args.baudrate,
            timeout=args.serial_timeout,
            )
        except (serial.SerialException, OSError) as exc:
            logging.error("Unable to open serial port! %s", exc)
            return
        # Flush any existing input
        ser.reset_input_buffer()

        with ser:
            # Keep reading until we find a valid CSV line with correct field count
            if args.skip_header:
                logging.info("Skipping until first valid CSV line is found...")
                while True:
                    raw = ser.readline().decode(errors="ignore").strip()
                    if not raw:
                        continue
                    try:
                        parse_csv_line(raw)
                        logging.info("Found first valid CSV line, starting data collection")
                        break
                    except ValueError:
                        logging.debug("Skipping: %s", raw[:80])  # Show first 80 chars
                        continue

            while True:
                raw = ser.readline().decode(errors="ignore").strip()
                if not raw:
                    time.sleep(args.idle_sleep)
                    continue

                try:
                    values = parse_csv_line(raw)
                except ValueError as exc:
                    logging.warning("Discarding malformed line: %s (%s)", raw, exc)
                    continue

                point = build_point(args.measurement, values)
                
                try:
                    client.write_points([point])
                    logging.debug("Written point to InfluxDB")
                except Exception as exc:
                    logging.error("Failed to write to InfluxDB: %s", exc)
                    time.sleep(args.error_backoff)
    finally:
        client.close()


def build_arg_parser() -> argparse.ArgumentParser:
    """Build command-line argument parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("serial_port", help="Serial device to read from, e.g. /dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200, help="Serial port baud rate")
    parser.add_argument("--serial-timeout", type=float, default=1.0, help="Serial read timeout in seconds")
    parser.add_argument("--skip-header", action="store_true", help="Skip the first header line")
    parser.add_argument("--idle-sleep", type=float, default=0.1, help="Sleep duration when no data is available")
    parser.add_argument("--error-backoff", type=float, default=1.0, help="Delay before retrying after write failure")
    
    # InfluxDB v1 settings
    parser.add_argument("--influx-host", default="localhost", help="InfluxDB host")
    parser.add_argument("--influx-port", type=int, default=8086, help="InfluxDB port")
    parser.add_argument("--influx-user", default="", help="InfluxDB username")
    parser.add_argument("--influx-password", default="", help="InfluxDB password")
    parser.add_argument("--influx-database", default="sensor_data", help="InfluxDB database name")
    parser.add_argument("--measurement", default="environment", help="InfluxDB measurement name")
    
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Logging verbosity",
    )
    return parser


def main() -> None:
    """Entry point."""
    parser = build_arg_parser()
    args = parser.parse_args()
    logging.basicConfig(
        level=getattr(logging, args.log_level),
        format="%(asctime)s %(levelname)s %(message)s",
        stream=sys.stderr,
    )

    signal.signal(signal.SIGINT, _graceful_shutdown)
    signal.signal(signal.SIGTERM, _graceful_shutdown)

    try:
        run(args)
    except KeyboardInterrupt:
        logging.info("Interrupted, exiting")


if __name__ == "__main__":
    main()
