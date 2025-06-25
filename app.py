from flask import Flask, render_template, request, jsonify, send_file
import sqlite3
from datetime import datetime
import pandas as pd

app = Flask(__name__)

# Database setup
DATABASE = 'sensor_data.db'

def get_db_connection():
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    with app.app_context():
        db = get_db_connection()
        db.execute('''
            CREATE TABLE IF NOT EXISTS sensor_data (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                timestamp DATETIME,
                temperature REAL,
                humidity REAL,
                gasValue INTEGER,
                voltage REAL,
                batteryLevel INTEGER,
                moisture1 INTEGER,
                moisture2 INTEGER,
                relayMotor INTEGER,
                relayWaterPump INTEGER,
                solarVoltage REAL
            )
        ''')
        db.commit()

@app.route('/')
def index():
    return render_template('dashboard.html')

@app.route('/dashboard')
def dashboard():
    return render_template('dashboard.html')

@app.route('/home')
def home():
    return render_template('home.html')

@app.route('/download')
def download():
    return render_template('download.html')

@app.route('/get_data')
def get_data():
    db = get_db_connection()
    data = db.execute('SELECT * FROM sensor_data ORDER BY timestamp DESC LIMIT 50').fetchall()
    db.close()
    return jsonify([dict(row) for row in data])

@app.route('/download_data')
def download_data():
    db = get_db_connection()
    data = db.execute('SELECT * FROM sensor_data').fetchall()
    db.close()
    df = pd.DataFrame(data)
    df.to_excel('sensor_data.xlsx', index=False)
    return send_file('sensor_data.xlsx', as_attachment=True)

@app.route('/reset_data', methods=['POST'])
def reset_data():
    db = get_db_connection()
    db.execute('DELETE FROM sensor_data')
    db.commit()
    db.close()
    return jsonify({"status": "success"})

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    db = get_db_connection()
    db.execute('''
        INSERT INTO sensor_data (timestamp, temperature, humidity, gasValue, voltage, batteryLevel, moisture1, moisture2, relayMotor, relayWaterPump, solarVoltage)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
    ''', (timestamp, data['temperature'], data['humidity'], data['gasValue'], data['voltage'], data['batteryLevel'], data['moisture1'], data['moisture2'], data['relayMotor'], data['relayWaterPump'], data['solarVoltage']))
    db.commit()
    db.close()
    return jsonify({"status": "success"}), 200

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000)