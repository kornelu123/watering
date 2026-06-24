CREATE EXTENSION IF NOT EXISTS timescaledb;

CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS user_settings (
    user_id INT PRIMARY KEY REFERENCES users(id) ON DELETE CASCADE,
    battery_threshold_percent INT DEFAULT 20,
    water_level_threshold_percent INT DEFAULT 10,
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS devices (
    device_id VARCHAR(50) PRIMARY KEY,
    user_id INT REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    water_duration_sec INT DEFAULT 10,
    moisture_threshold_percent INT DEFAULT 30,
    created_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS device_groups (
    id SERIAL PRIMARY KEY,
    user_id INT REFERENCES users(id) ON DELETE CASCADE,
    name VARCHAR(100) NOT NULL,
    moisture_threshold_percent INT DEFAULT 30,
    watering_duration_sec INT DEFAULT 10,
    created_at TIMESTAMPTZ DEFAULT NOW(),
    UNIQUE (user_id, name)
);

ALTER TABLE devices
    ADD COLUMN IF NOT EXISTS group_id INT REFERENCES device_groups(id) ON DELETE SET NULL;

CREATE TABLE IF NOT EXISTS telemetry (
    time TIMESTAMPTZ NOT NULL,
    device_id VARCHAR(50) REFERENCES devices(device_id) ON DELETE CASCADE,
    water_lvl INT NOT NULL,
    battery_lvl INT NOT NULL,
    moisture_lvl INT NOT NULL,
    uptime INT NOT NULL
);

-- 3. Inicjalizacja TimescaleDB
SELECT create_hypertable('telemetry', 'time');
SELECT add_retention_policy('telemetry', INTERVAL '90 days');
CREATE INDEX ix_telemetry_device_id_time ON telemetry (device_id, time DESC);

-- 4. Tworzenie Zmaterializowanych Widoków
CREATE MATERIALIZED VIEW telemetry_15m WITH (timescaledb.continuous) AS
SELECT
    time_bucket('15 minutes', time) AS bucket,
    device_id,
    ROUND((AVG(moisture_lvl) / 65535.0) * 100, 2) AS avg_moisture,
    ROUND((MIN(moisture_lvl) / 65535.0) * 100, 2) AS min_moisture,
    ROUND((MAX(moisture_lvl) / 65535.0) * 100, 2) AS max_moisture,
    ROUND((AVG(battery_lvl) / 65535.0) * 100, 2) AS avg_battery,
    ROUND((MIN(battery_lvl) / 65535.0) * 100, 2) AS min_battery,
    ROUND((MAX(battery_lvl) / 65535.0) * 100, 2) AS max_battery,
    ROUND((AVG(water_lvl) / 65535.0) * 100, 2)   AS avg_water,
    ROUND((MIN(water_lvl) / 65535.0) * 100, 2)   AS min_water,
    ROUND((MAX(water_lvl) / 65535.0) * 100, 2)   AS max_water
FROM telemetry
GROUP BY bucket, device_id;

CREATE MATERIALIZED VIEW telemetry_1h WITH (timescaledb.continuous) AS
SELECT
    time_bucket('1 hour', time) AS bucket,
    device_id,
    ROUND((AVG(moisture_lvl) / 65535.0) * 100, 2) AS avg_moisture,
    ROUND((MIN(moisture_lvl) / 65535.0) * 100, 2) AS min_moisture,
    ROUND((MAX(moisture_lvl) / 65535.0) * 100, 2) AS max_moisture,
    ROUND((AVG(battery_lvl) / 65535.0) * 100, 2) AS avg_battery,
    ROUND((MIN(battery_lvl) / 65535.0) * 100, 2) AS min_battery,
    ROUND((MAX(battery_lvl) / 65535.0) * 100, 2) AS max_battery,
    ROUND((AVG(water_lvl) / 65535.0) * 100, 2)   AS avg_water,
    ROUND((MIN(water_lvl) / 65535.0) * 100, 2)   AS min_water,
    ROUND((MAX(water_lvl) / 65535.0) * 100, 2)   AS max_water
FROM telemetry
GROUP BY bucket, device_id;

CREATE MATERIALIZED VIEW telemetry_1d WITH (timescaledb.continuous) AS
SELECT
    time_bucket('1 day', time) AS bucket,
    device_id,
    ROUND((AVG(moisture_lvl) / 65535.0) * 100, 2) AS avg_moisture,
    ROUND((MIN(moisture_lvl) / 65535.0) * 100, 2) AS min_moisture,
    ROUND((MAX(moisture_lvl) / 65535.0) * 100, 2) AS max_moisture,
    ROUND((AVG(battery_lvl) / 65535.0) * 100, 2) AS avg_battery,
    ROUND((MIN(battery_lvl) / 65535.0) * 100, 2) AS min_battery,
    ROUND((MAX(battery_lvl) / 65535.0) * 100, 2) AS max_battery,
    ROUND((AVG(water_lvl) / 65535.0) * 100, 2)   AS avg_water,
    ROUND((MIN(water_lvl) / 65535.0) * 100, 2)   AS min_water,
    ROUND((MAX(water_lvl) / 65535.0) * 100, 2)   AS max_water
FROM telemetry
GROUP BY bucket, device_id;

-- Widok 15-minutowy: kompromis 5 min offset / 10 min schedule
SELECT add_continuous_aggregate_policy('telemetry_15m',
    start_offset => INTERVAL '3 days',
    end_offset => INTERVAL '5 minutes',
    schedule_interval => INTERVAL '10 minutes');

-- Widok godzinny: 20 min offset / 40 min schedule
SELECT add_continuous_aggregate_policy('telemetry_1h',
    start_offset => INTERVAL '4 weeks',
    end_offset => INTERVAL '20 minutes',
    schedule_interval => INTERVAL '40 minutes');

-- Widok dzienny: 8 h offset / 16 h schedule
SELECT add_continuous_aggregate_policy('telemetry_1d',
    start_offset => INTERVAL '6 months',
    end_offset => INTERVAL '8 hours',
    schedule_interval => INTERVAL '16 hours');