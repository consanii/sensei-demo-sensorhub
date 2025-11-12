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
import configparser
from pathlib import Path

import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS

# Load ./config.ini using global path
_ini_path = Path(__file__).resolve().parent / "config.ini"
if _ini_path.exists():
    _cfg = configparser.ConfigParser()
    _cfg.read(_ini_path)
    token = _cfg.get("influx", "token")
    org = _cfg.get("influx", "org")
    url = _cfg.get("influx", "url")
    bucket = _cfg.get("influx", "bucket")
    measurement = _cfg.get("influx", "measurement")
else:
    raise RuntimeError("Configuration file ./config.ini not found!")

# CSV field order, as specified in the sensorhub firmware
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
    # Initialize InfluxDB v2 client
    try:
        logging.info("Connecting to InfluxDB at %s", url)
        write_client = influxdb_client.InfluxDBClient(url=url, token=token, org=org)
        write_api = write_client.write_api(write_options=SYNCHRONOUS)
    except Exception as exc:
        logging.error("Failed to connect to InfluxDB: %s", exc)
        return
    
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
            sys.exit(1)
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
                        # Log the full raw line at DEBUG so operators can inspect problems
                        logging.debug("Skipping malformed header candidate: %s", raw)
                        continue


            # ------------- MAIN LOOP -------------
            while True:
                # Read a line from serial port
                try:
                    raw = ser.readline().decode(errors="ignore").strip()
                except (serial.SerialException, OSError) as exc:
                    logging.error("Serial port error: %s", exc)
                    sys.exit(1)
                if not raw:
                    time.sleep(args.idle_sleep)
                    continue

                # [DEBUG] Log the full CSV line for troubleshooting
                logging.debug("CSV line: %s", raw)

                # Parse CSV line
                try:
                    values = parse_csv_line(raw)
                except ValueError as exc:
                    logging.warning("Discarding malformed line: %s", exc)
                    continue
                # Build InfluxDB point
                point = build_point(measurement, values)
                # Write point to InfluxDB
                try:
                    write_api.write(bucket=bucket, org=org, record=point)
                    logging.debug("Written point to InfluxDB")
                except Exception as exc:
                    logging.error("Failed to write to InfluxDB: %s", exc)
    finally:
        # Close the InfluxDB client created above
        try:
            write_client.close()
        except Exception:
            logging.error("Failed to close InfluxDB client on exit")
            pass


def build_arg_parser() -> argparse.ArgumentParser:
    """Build command-line argument parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("serial_port", help="Serial device to read from, e.g. /dev/ttyUSB0")
    parser.add_argument("--baudrate", type=int, default=115200, help="Serial port baud rate")
    parser.add_argument("--serial-timeout", type=float, default=1.0, help="Serial read timeout in seconds")
    parser.add_argument("--skip-header", action="store_true", help="Skip the first header line")
    parser.add_argument("--idle-sleep", type=float, default=0.1, help="Sleep duration when no data is available")
    
    parser.add_argument(
        "--log-level",
        default="INFO",
        choices=["DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"],
        help="Logging verbosity",
    )
    return parser




# --------------- Main ---------------
def main() -> None:
    parser = build_arg_parser()
    args = parser.parse_args()
    logging.basicConfig(
        level=getattr(logging, args.log_level),
        format="%(asctime)s %(levelname)s %(message)s",
        stream=sys.stderr,
    )

    signal.signal(signal.SIGINT, _graceful_shutdown)
    signal.signal(signal.SIGTERM, _graceful_shutdown)

    # Print InfluxDB config at debug level
    logging.debug("InfluxDB config: org=%s url=%s bucket=%s", org, url, bucket)

    try:
        run(args)
    except KeyboardInterrupt: # Redundant, but safe
        logging.info("Interrupted, exiting")


if __name__ == "__main__":
    main()
