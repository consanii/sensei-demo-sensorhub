# Serial to Database (InfluxDB) Bridge

This folder contains a Python script and supporting configuration files to stream SENSEI sensor data from a serial port directly into an InfluxDB time-series database. Currently, it supports InfluxDB v2 and the serial port is hard coded to `/dev/ttyACM0`.


## Setup
To function properly, the script requires a configuration file (`config.ini`) to specify InfluxDB connection details. Some dependencies must also be installed.

### Dependencies

Required Python packages:
- `pyserial`: Serial port communication
- `influxdb-client`: InfluxDB v2 Python client

Install dependencies:
```bash
pip install pyserial influxdb-client
```

### config.ini Template

Create a `config.ini` file with the following structure, **at the same level as the `serial_to_db.py` script**:

```ini
# InfluxDB v2 Configuration
# Fill in your real values below

[influx]
# InfluxDB v2 API token (generate from InfluxDB UI)
token = YOUR_INFLUXDB_TOKEN_HERE

# Organization name
org = YOUR_ORG_NAME

# InfluxDB URL (including port if non-standard)
url = URL_TO_DB

# Bucket name where data will be stored
bucket = YOUR_BUCKET_NAME

# Measurement name for the data points
measurement = YOUR_MEASUREMENT_NAME
```

### Configuration Parameters

- **token**: Your InfluxDB v2 API authentication token
- **org**: The organization name in InfluxDB
- **url**: The InfluxDB server URL (default: `http://127.0.0.1:8086` for local instance)
- **bucket**: The target bucket name for storing sensor data
- **measurement**: The measurement name used for all data points

## Usage

### Manual Execution

Run the script manually:

```bash
python serial_to_db.py /dev/ttyACM0 --baudrate 115200 --skip-header
```

**Command-line arguments:**
- `serial_port`: Serial device path (e.g., `/dev/ttyACM0`, `/dev/ttyUSB0`)
- `--baudrate`: Serial baud rate (default: 115200)
- `--serial-timeout`: Read timeout in seconds (default: 1.0)
- `--skip-header`: Skip initial lines until first valid CSV data is found
- `--idle-sleep`: Sleep duration when no data available (default: 0.1s)
- `--log-level`: Logging verbosity (DEBUG, INFO, WARNING, ERROR, CRITICAL)

### Running as a System Service

For continuous operation, the script can be installed as a systemd service. Follow the steps below to set it up.

## Systemd Service File

### serial_to_db.service Template

Create or modify the service file at `/etc/systemd/system/serial_to_db.service`:

```ini
[Unit]
Description=Serial to InfluxDB data logger for sensorhub
After=YOUR_SERIAL_DEVICE.device

[Service]
Type=exec

# Logging configuration - logs go to systemd journal
StandardOutput=journal
StandardError=journal
SyslogIdentifier=serial_to_db

# User and working directory (adjust to your setup)
User=YOUR_USERNAME
WorkingDirectory=/path/to/sensei-demo-sensorhub

# Command to execute (adjust paths to match your environment)
ExecStart=/path/to/conda run --no-capture-output -n YOUR_ENV_NAME python /path/to/sensei-demo-sensorhub/serial_to_db/serial_to_db.py /dev/ttyACM0 --skip-header

# Auto-restart configuration
Restart=always
RestartSec=60
# Unlimited restart attempts (systemd default is 5 attempts in 10 seconds)
StartLimitBurst=0

[Install]
WantedBy=multi-user.target
```

### Service Management

```bash
# Install the service
sudo cp serial_to_db.service /etc/systemd/system/
sudo systemctl daemon-reload

# Enable auto-start on boot
sudo systemctl enable serial_to_db

# Start the service
sudo systemctl start serial_to_db

# Check status
sudo systemctl status serial_to_db

# View logs
sudo journalctl -u serial_to_db -f
```