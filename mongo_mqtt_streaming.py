import pymongo
import paho.mqtt.client as mqtt
import json
import ssl
from datetime import datetime, timezone
from pymongo.mongo_client import MongoClient
from pymongo.server_api import ServerApi
from cryptography.fernet import Fernet

# Encryption/Decryption Key
# This key must be securely stored and shared with the MQTT publisher
SECRET_KEY = b'your-32-byte-secret-key-here'  # Use a secure key
cipher = Fernet(SECRET_KEY)

# MongoDB URI with secure connection
uri = "mongodb+srv://dlau:gvptZb3ULYsCxZO9@cluster-357.2bqlu.mongodb.net/?retryWrites=true&w=majority&appName=Cluster-357"
# Create a new client and connect to the server
mongo_client = pymongo.MongoClient(uri, server_api=ServerApi('1'))

# Send a ping to confirm a successful connection
try:
    mongo_client.admin.command('ping')
    print("Pinged your deployment. You successfully connected to MongoDB!")
except Exception as e:
    print(e)

# MongoDB configuration
db = mongo_client["smart_carpark"]
collection = db["realtime_data"]

# MQTT configuration
mqtt_broker_address = "your_mqtt_broker_address_here"
mqtt_port = 8883  # Secure MQTT port
mqtt_topic = "iot"

# MQTT authentication credentials
mqtt_username = "your_mqtt_username"
mqtt_password = "your_mqtt_password"

# Define the callback function for connection
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"Successfully connected")
        client.subscribe(mqtt_topic)
    else:
        print(f"Connection failed with reason code {reason_code}")

# Define the callback function for ingesting data into MongoDB
def on_message(client, userdata, message):
    try:
        # Decode and decrypt the payload
        encrypted_payload = message.payload
        decrypted_payload = cipher.decrypt(encrypted_payload).decode("utf-8")
        print(f"Received and decrypted message: {decrypted_payload}")

        # Convert MQTT timestamp to datetime
        timestamp = datetime.now(timezone.utc)
        datetime_obj = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")

        # Parse JSON data
        data = json.loads(decrypted_payload)

        # Assign values to variables
        TotalCar = data["TotalCar"]
        EvCar = data["EvCar"]
        PetrolCar = data["PetrolCar"]
        AirQuality = data["AirQuality"]
        Parking1Full = data["Parking1Full"]
        Parking2Full = data["Parking2Full"]

        # Process the payload and insert into MongoDB with proper timestamp
        document = {
            "timestamp": datetime_obj,
            "TotalCar": TotalCar,
            "EvCar": EvCar,
            "PetrolCar": PetrolCar,
            "AirQuality": AirQuality,
            "Parking1Full": Parking1Full,
            "Parking2Full": Parking2Full,
        }
        collection.insert_one(document)
        print("Data ingested into MongoDB")
    except Exception as e:
        print(f"Error processing message: {e}")

# Create a MQTT client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Configure TLS for secure connection
client.tls_set(
    ca_certs="path_to_ca_certificate.pem",  # Path to the CA certificate
    certfile=None,
    keyfile=None,
    cert_reqs=ssl.CERT_REQUIRED,
    tls_version=ssl.PROTOCOL_TLS,
    ciphers=None,
)

# Set MQTT authentication credentials
client.username_pw_set(mqtt_username, mqtt_password)

# Attach the callbacks using explicit methods
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, mqtt_port, 60)

# Start the MQTT loop
client.loop_forever()
